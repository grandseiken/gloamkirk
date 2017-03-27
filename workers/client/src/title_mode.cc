#include "workers/client/src/title_mode.h"
#include "workers/client/src/connect_mode.h"
#include "workers/client/src/input.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/text.h"
#include "workers/client/src/shaders/title.h"
#include <SFML/Window.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdint>
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

template <typename T, typename U>
void select_menu(T& item, U count, std::int8_t add) {
  auto c = static_cast<std::int32_t>(count);
  item = static_cast<T>((static_cast<std::int32_t>(item) + 256 * c + add) % c);
}
}  // anonymous

TitleMode::TitleMode(ModeState& mode_state, bool fade_in,
                     const worker::ConnectionParameters& connection_params,
                     const worker::LocatorParameters& locator_params)
: mode_state_{mode_state}
, connection_params_{connection_params}
, locator_{kLocatorHost, locator_params}
, title_{gloam::load_texture("assets/title.png")}
, title_program_{"title",
                 {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                 {"title_fragment", GL_FRAGMENT_SHADER, shaders::title_fragment}} {
  title_.texture.set_linear();

  if (fade_in) {
    fade_in_ = 32;
  }
}

void TitleMode::update(const Input& input) {
  if (connection_future_) {
    if (input.pressed(Button::kCancel)) {
      // Doesn't seem to work most of the time, can't cancel once we're past the locator.
      connection_cancel_ = true;
    }
  } else if (deployment_list_) {
    if (input.pressed(Button::kUp)) {
      select_menu(deployment_choice_, deployment_list_->Deployments.size(), -1);
    }
    if (input.pressed(Button::kDown)) {
      select_menu(deployment_choice_, deployment_list_->Deployments.size(), 1);
    }
    if (input.pressed(Button::kConfirm)) {
      connection_future_.reset(new ConnectionFutureWrapper{locator_.ConnectAsync(
          deployment_list_->Deployments[deployment_choice_].DeploymentName, connection_params_,
          [&](const worker::QueueStatus& status) {
            if (status.Error) {
              finish_connect_frame_ = mode_state_.frame;
              queue_status_error_ = *status.Error;
              return false;
            }

            new_queue_status_ = "Position in queue: " + std::to_string(status.PositionInQueue);
            return !connection_cancel_;
          })});
      connect_frame_ = mode_state_.frame;
    }
  } else if (mode_state_.settings_menu) {
    if (input.pressed(Button::kUp)) {
      select_menu(mode_state_.settings_item, SettingsItem::kCount, -1);
    }
    if (input.pressed(Button::kDown)) {
      select_menu(mode_state_.settings_item, SettingsItem::kCount, 1);
    }
    if (input.pressed(Button::kConfirm)) {
      if (mode_state_.settings_item == SettingsItem::kToggleFullscreen) {
        mode_state_.fullscreen = !mode_state_.fullscreen;
      } else if (mode_state_.settings_item == SettingsItem::kAntialiasLevel) {
      } else if (mode_state_.settings_item == SettingsItem::kBack) {
        mode_state_.settings_menu = false;
      }
    }
    if (input.pressed(Button::kCancel)) {
      mode_state_.settings_menu = false;
    }
  } else if (text_alpha(mode_state_.frame) > 0.f && !locator_future_) {
    if (input.pressed(Button::kUp)) {
      select_menu(mode_state_.menu_item, MenuItem::kCount, -1);
    }
    if (input.pressed(Button::kDown)) {
      select_menu(mode_state_.menu_item, MenuItem::kCount, 1);
    }
    if (input.pressed(Button::kConfirm)) {
      if (mode_state_.menu_item == MenuItem::kConnect && mode_state_.connect_local) {
        connection_future_.reset(new ConnectionFutureWrapper{
            worker::Connection::ConnectAsync(kLocalhost, kLocalPort, connection_params_)});
        connect_frame_ = mode_state_.frame;
      } else if (mode_state_.menu_item == MenuItem::kConnect && !mode_state_.connect_local) {
        locator_future_.reset(new LocatorFutureWrapper{locator_.GetDeploymentListAsync()});
        connect_frame_ = mode_state_.frame;
      } else if (mode_state_.menu_item == MenuItem::kSettings) {
        mode_state_.settings_menu = true;
      } else if (mode_state_.menu_item == MenuItem::kExitApplication) {
        mode_state_.exit_application = true;
      }
    }
  }

  if (fade_in_) {
    --fade_in_;
  }
  ++mode_state_.frame;
  bool connection_poll =
      mode_state_.frame - connect_frame_ > 0 && (mode_state_.frame - connect_frame_) % 64 == 0;
  if (finish_connect_frame_ && mode_state_.frame - finish_connect_frame_ >= 32) {
    if (!new_queue_status_.empty()) {
      queue_status_ = new_queue_status_;
      new_queue_status_.clear();
    }

    if (!queue_status_error_.empty()) {
      mode_state_.new_mode.reset(new ConnectMode{mode_state_, queue_status_error_});
    } else if (connection_future_) {
      mode_state_.new_mode.reset(new ConnectMode{mode_state_, connection_future_->future.Get()});
    } else if (deployment_list_) {
      mode_state_.new_mode.reset(new ConnectMode{mode_state_, *deployment_list_->Error});
    }
  } else if (locator_future_ && connection_poll && locator_future_->future.Wait({0})) {
    deployment_list_.reset(new worker::DeploymentList{locator_future_->future.Get()});
    locator_future_.reset();

    if (deployment_list_->Error) {
      finish_connect_frame_ = mode_state_.frame;
    } else if (deployment_list_->Deployments.empty()) {
      finish_connect_frame_ = mode_state_.frame;
      deployment_list_->Error.emplace("No deployments found.");
    } else {
      fade_in_ = 32;
    }
  } else if (!finish_connect_frame_ && connection_future_ && connection_poll &&
             connection_future_->future.Wait({0})) {
    finish_connect_frame_ = mode_state_.frame;
  }
}

void TitleMode::render(const Renderer& renderer) const {
  glm::vec2 dimensions = renderer.framebuffer_dimensions();
  auto max_texture_scale = (dimensions - glm::vec2{60.f, 180.f}) / glm::vec2{title_.dimensions};
  float texture_scale = std::min(max_texture_scale.x, max_texture_scale.y);
  auto scaled_dimensions = texture_scale * glm::vec2{title_.dimensions};
  auto border = (dimensions - scaled_dimensions) / glm::vec2{2.f, 6.f};
  border.y = std::min(border.y, border.x);

  {
    auto alpha = title_alpha(mode_state_.frame);
    if (connection_future_ && deployment_list_) {
      alpha = 0.f;
    } else if (connection_future_ || locator_future_ || deployment_list_) {
      alpha *= std::max(
          0.f, std::min(1.f, 1.f - static_cast<float>(mode_state_.frame - connect_frame_) / 64.f));
    }
    auto fade = 1.f;
    if (!deployment_list_ && fade_in_) {
      fade = std::max(0.f, std::min(1.f, 1.f - static_cast<float>(fade_in_) / 32.f));
    } else if (finish_connect_frame_) {
      fade = std::max(
          0.f, std::min(1.f, 1.f -
                            static_cast<float>(mode_state_.frame - finish_connect_frame_) / 32.f));
    }

    auto program = title_program_.use();
    glUniform1f(program.uniform("fade"), fade);
    glUniform1f(program.uniform("frame"), static_cast<float>(mode_state_.frame));
    glUniform1f(program.uniform("random_seed"), static_cast<float>(mode_state_.random_seed));
    glUniform1f(program.uniform("title_alpha"), alpha);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    glUniform2fv(program.uniform("scaled_dimensions"), 1, glm::value_ptr(scaled_dimensions));
    glUniform2fv(program.uniform("border"), 1, glm::value_ptr(border));
    program.uniform_texture("title_texture", title_.texture);
    renderer.set_simplex3_uniforms(program);
    renderer.draw_quad();
  }

  auto max_items = std::max(static_cast<std::int32_t>(MenuItem::kCount),
                            static_cast<std::int32_t>(SettingsItem::kCount));
  auto menu_height = dimensions.y -
      (dimensions.y - border.y - scaled_dimensions.y + max_items * shaders::text_height) / 2;
  auto menu_fade = text_alpha(mode_state_.frame);
  auto draw_menu_item = [&](const std::string& text, std::int32_t item, std::int32_t choice) {
    auto text_width = renderer.text_width(text);
    renderer.draw_text(
        text, {dimensions.x / 2 - text_width / 2, menu_height + item * shaders::text_height},
        item == choice ? glm::vec4{.75f, .75f, .75f, menu_fade}
                       : glm::vec4{.75f, .75f, .75f, .25f * menu_fade});
  };

  if (!connection_future_ && !locator_future_ && !deployment_list_) {
    if (mode_state_.settings_menu) {
      draw_menu_item("TOGGLE FULLSCREEN",
                     static_cast<std::int32_t>(SettingsItem::kToggleFullscreen),
                     static_cast<std::int32_t>(mode_state_.settings_item));
      draw_menu_item("ANTIALIAS LEVEL", static_cast<std::int32_t>(SettingsItem::kAntialiasLevel),
                     static_cast<std::int32_t>(mode_state_.settings_item));
      draw_menu_item("BACK", static_cast<std::int32_t>(SettingsItem::kBack),
                     static_cast<std::int32_t>(mode_state_.settings_item));
    } else {
      draw_menu_item("CONNECT", static_cast<std::int32_t>(MenuItem::kConnect),
                     static_cast<std::int32_t>(mode_state_.menu_item));
      draw_menu_item("SETTINGS", static_cast<std::int32_t>(MenuItem::kSettings),
                     static_cast<std::int32_t>(mode_state_.menu_item));
      draw_menu_item("EXIT", static_cast<std::int32_t>(MenuItem::kExitApplication),
                     static_cast<std::int32_t>(mode_state_.menu_item));
    }
  }

  if (!connection_future_ && deployment_list_ && !deployment_list_->Error) {
    menu_height = dimensions.y / 2;
    menu_fade = std::max(0.f, std::min(1.f, 1.f - static_cast<float>(fade_in_) / 32.f));

    std::string text = "Choose deployment:";
    auto text_width = renderer.text_width(text);
    renderer.draw_text(
        text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2 - 2 * shaders::text_height},
        glm::vec4{.75f, .75f, .75f, .5f * menu_fade});

    std::int32_t i = 0;
    for (const auto& deployment : deployment_list_->Deployments) {
      text = deployment.DeploymentName;
      if (!deployment.Description.empty()) {
        text += " - " + deployment.Description;
      }
      draw_menu_item(text, i, deployment_choice_);
      ++i;
    }
  }

  if ((connection_future_ || locator_future_) && !finish_connect_frame_) {
    auto alpha = static_cast<float>((mode_state_.frame - connect_frame_) % 64) / 64.f;

    auto text = !connection_future_ ? "SEARCHING..." : queue_status_.empty() ? "CONNECTING..."
                                                                             : queue_status_;
    auto text_width = renderer.text_width(text);
    renderer.draw_text(text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, alpha});
  }
}

}  // ::gloam
