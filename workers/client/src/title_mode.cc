#include "workers/client/src/title_mode.h"
#include "workers/client/src/connect_mode.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/text.h"
#include "workers/client/src/shaders/title.h"
#include <SFML/Window.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>
#include <string>

namespace gloam {
namespace {
const std::uint16_t kLocalPort = 7777;
const std::string kLocalhost = "127.0.0.1";
const std::string kLocatorHost = "locator.improbable.io";

float title_alpha(std::uint64_t frame) {
  return std::min(1.f, std::max(0.f, (static_cast<float>(frame) - 64.f) / 256.f));
}

float text_alpha(std::uint64_t frame) {
  return std::min(1.f, std::max(0.f, (static_cast<float>(frame) - 256.f + 64.f) / 64.f));
}

std::int32_t random_seed_ = 0;
std::uint64_t frame_ = 0;
}  // anonymous

TitleMode::TitleMode(bool first_run, bool fade_in, bool local,
                     const worker::ConnectionParameters& connection_params,
                     const worker::LocatorParameters& locator_params)
: title_{gloam::load_texture("assets/title.png")}
, title_program_{"title",
                 {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                 {"title_fragment", GL_FRAGMENT_SHADER, shaders::title_fragment}}
, connection_local_{local}
, connection_params_{connection_params}
, locator_{kLocatorHost, locator_params} {
  title_.texture.set_linear();

  if (first_run) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    std::mt19937 generator{static_cast<unsigned int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count())};
    std::uniform_int_distribution<std::int32_t> distribution(0, 1 << 16);

    random_seed_ = distribution(generator);
    frame_ = 0;
  }

  if (fade_in) {
    fade_in_ = 32;
  } else {
    menu_item_ = kToggleFullscreen;
  }
}

ModeResult TitleMode::event(const sf::Event& event) {
  if (connection_future_) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
      // Doesn't seem to work.
      connection_cancel_ = true;
    }
    return {ModeAction::kNone, {}};
  }

  if (deployment_list_) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Up) {
      deployment_choice_ = (deployment_choice_ + deployment_list_->Deployments.size() - 1) %
          static_cast<std::int32_t>(deployment_list_->Deployments.size());
    } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Down) {
      deployment_choice_ = (deployment_choice_ + 1) % deployment_list_->Deployments.size();
    } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return) {
      connection_future_.reset(new ConnectionFutureWrapper{locator_.ConnectAsync(
          deployment_list_->Deployments[deployment_choice_].DeploymentName, connection_params_,
          [&](const worker::QueueStatus& status) {
            if (status.Error) {
              finish_connect_frame_ = frame_;
              queue_status_error_ = *status.Error;
              return false;
            }

            new_queue_status_ = "Position in queue: " + std::to_string(status.PositionInQueue);
            return !connection_cancel_;
          })});
      connect_frame_ = frame_;
    }
    return {ModeAction::kNone, {}};
  }

  if (text_alpha(frame_) <= 0.f || locator_future_) {
    return {ModeAction::kNone, {}};
  }

  if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Up) {
    menu_item_ = (menu_item_ + kMenuItemCount - 1) % kMenuItemCount;
  } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Down) {
    menu_item_ = (menu_item_ + 1) % kMenuItemCount;
  } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return) {
    if (menu_item_ == kConnect && connection_local_) {
      connection_future_.reset(new ConnectionFutureWrapper{
          worker::Connection::ConnectAsync(kLocalhost, kLocalPort, connection_params_)});
      connect_frame_ = frame_;
    } else if (menu_item_ == kConnect && !connection_local_) {
      locator_future_.reset(new LocatorFutureWrapper{locator_.GetDeploymentListAsync()});
      connect_frame_ = frame_;
    } else if (menu_item_ == kToggleFullscreen) {
      return {ModeAction::kToggleFullscreen, {}};
    } else if (menu_item_ == kExitApplication) {
      return {ModeAction::kExitApplication, {}};
    }
  }
  return {ModeAction::kNone, {}};
}

ModeResult TitleMode::update() {
  if (fade_in_) {
    --fade_in_;
  }
  ++frame_;
  bool connection_poll = frame_ - connect_frame_ > 0 && (frame_ - connect_frame_) % 64 == 0;
  if (finish_connect_frame_ && frame_ - finish_connect_frame_ >= 32) {
    if (!new_queue_status_.empty()) {
      queue_status_ = new_queue_status_;
      new_queue_status_.clear();
    }

    if (!queue_status_error_.empty()) {
      return {{}, std::unique_ptr<Mode>{new ConnectMode{queue_status_error_}}};
    } else if (connection_future_) {
      return {{}, std::unique_ptr<Mode>{new ConnectMode{connection_future_->future.Get()}}};
    } else if (deployment_list_) {
      return {{}, std::unique_ptr<Mode>{new ConnectMode{*deployment_list_->Error}}};
    }
  } else if (locator_future_ && connection_poll && locator_future_->future.Wait({0})) {
    deployment_list_.reset(new worker::DeploymentList{locator_future_->future.Get()});
    locator_future_.reset();

    if (deployment_list_->Error) {
      finish_connect_frame_ = frame_;
    } else if (deployment_list_->Deployments.empty()) {
      finish_connect_frame_ = frame_;
      deployment_list_->Error.emplace("No deployments found.");
    } else {
      fade_in_ = 32;
    }
  } else if (!finish_connect_frame_ && connection_future_ && connection_poll &&
             connection_future_->future.Wait({0})) {
    finish_connect_frame_ = frame_;
  }
  return {};
}

void TitleMode::render(const Renderer& renderer) const {
  glm::vec2 dimensions = renderer.framebuffer_dimensions();
  auto max_texture_scale = (dimensions - glm::vec2{60.f, 180.f}) / title_.dimensions;
  float texture_scale = std::min(max_texture_scale.x, max_texture_scale.y);
  auto scaled_dimensions = texture_scale * title_.dimensions;
  auto border = (dimensions - scaled_dimensions) / glm::vec2{2.f, 6.f};
  border.y = std::min(border.y, border.x);

  {
    auto alpha = title_alpha(frame_);
    if (connection_future_ && deployment_list_) {
      alpha = 0.f;
    } else if (connection_future_ || locator_future_ || deployment_list_) {
      alpha *=
          std::max(0.f, std::min(1.f, 1.f - static_cast<float>(frame_ - connect_frame_) / 64.f));
    }
    auto fade = 1.f;
    if (!deployment_list_ && fade_in_) {
      fade = std::max(0.f, std::min(1.f, 1.f - static_cast<float>(fade_in_) / 32.f));
    } else if (finish_connect_frame_) {
      fade = std::max(
          0.f, std::min(1.f, 1.f - static_cast<float>(frame_ - finish_connect_frame_) / 32.f));
    }

    auto program = title_program_.use();
    glUniform1f(program.uniform("fade"), fade);
    glUniform1f(program.uniform("frame"), static_cast<float>(frame_));
    glUniform1f(program.uniform("random_seed"), static_cast<float>(random_seed_));
    glUniform1f(program.uniform("title_alpha"), alpha);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    glUniform2fv(program.uniform("scaled_dimensions"), 1, glm::value_ptr(scaled_dimensions));
    glUniform2fv(program.uniform("border"), 1, glm::value_ptr(border));
    program.uniform_texture("title_texture", title_.texture);
    renderer.set_simplex3_uniforms(program);
    renderer.draw_quad();
  }

  auto menu_height = dimensions.y -
      (dimensions.y - border.y - scaled_dimensions.y + kMenuItemCount * shaders::text_height) / 2;
  auto draw_menu_item = [&](const std::string& text, std::int32_t item, std::int32_t choice,
                            float height, float fade) {
    auto text_width = renderer.text_width(text);
    renderer.draw_text(text,
                       {dimensions.x / 2 - text_width / 2, height + item * shaders::text_height},
                       item == choice ? glm::vec4{.75f, .75f, .75f, fade}
                                      : glm::vec4{.75f, .75f, .75f, .25f * fade});
  };

  if (!connection_future_ && !locator_future_ && !deployment_list_) {
    draw_menu_item("CONNECT", kConnect, menu_item_, menu_height, text_alpha(frame_));
    draw_menu_item("TOGGLE FULLSCREEN", kToggleFullscreen, menu_item_, menu_height,
                   text_alpha(frame_));
    draw_menu_item("EXIT", kExitApplication, menu_item_, menu_height, text_alpha(frame_));
  }

  if (!connection_future_ && deployment_list_ && !deployment_list_->Error) {
    auto fade = std::max(0.f, std::min(1.f, 1.f - static_cast<float>(fade_in_) / 32.f));

    std::string text = "Choose deployment:";
    auto text_width = renderer.text_width(text);
    renderer.draw_text(
        text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2 - 2 * shaders::text_height},
        glm::vec4{.75f, .75f, .75f, .5f * fade});

    std::int32_t i = 0;
    for (const auto& deployment : deployment_list_->Deployments) {
      text = deployment.DeploymentName;
      if (!deployment.Description.empty()) {
        text += " - " + deployment.Description;
      }
      draw_menu_item(text, i, deployment_choice_, dimensions.y / 2, fade);
      ++i;
    }
  }

  if ((connection_future_ || locator_future_) && !finish_connect_frame_) {
    auto alpha = static_cast<float>((frame_ - connect_frame_) % 64) / 64.f;

    auto text = !connection_future_ ? "SEARCHING..." : queue_status_.empty() ? "CONNECTING..."
                                                                             : queue_status_;
    auto text_width = renderer.text_width(text);
    renderer.draw_text(text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, alpha});
  }
}

}  // ::gloam
