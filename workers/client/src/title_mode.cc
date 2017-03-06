#include "workers/client/src/title_mode.h"
#include "workers/client/src/connect_mode.h"
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
  return std::min(1.f, std::max(0.f, (static_cast<float>(frame) - 256.f - 64.f) / 16.f));
}

worker::Future<worker::Connection> connect(bool local,
                                           const worker::ConnectionParameters& connection_params,
                                           const worker::LocatorParameters& locator_params) {
  if (local) {
    return worker::Connection::ConnectAsync(kLocalhost, kLocalPort, connection_params);
  } else {
    worker::Locator locator{kLocatorHost, locator_params};
    return locator.ConnectAsync("test", connection_params,
                                [](const worker::QueueStatus&) { return true; });
  }
}

}  // anonymous

TitleMode::TitleMode(bool first_run, bool local,
                     const worker::ConnectionParameters& connection_params,
                     const worker::LocatorParameters& locator_params)
: title_{gloam::load_texture("assets/title.png")}
, title_program_{"title",
                 {"title_vertex", GL_VERTEX_SHADER, shaders::title_vertex},
                 {"title_fragment", GL_FRAGMENT_SHADER, shaders::title_fragment}}
, connection_local_{local}
, connection_params_{connection_params}
, locator_params_{locator_params} {
  title_.texture.set_linear();
  auto now = std::chrono::system_clock::now().time_since_epoch();
  std::mt19937 generator{static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count())};
  std::uniform_int_distribution<std::int32_t> distribution(0, 1 << 16);
  random_seed_ = distribution(generator);

  if (!first_run) {
    frame_ = 1024;
  }
}

ModeAction TitleMode::event(const sf::Event& event) {
  if (title_alpha(frame_) < 1.f || connection_future_) {
    return ModeAction::kNone;
  }

  if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Up) {
    menu_item_ = (menu_item_ + kMenuItemCount - 1) % kMenuItemCount;
  } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Down) {
    menu_item_ = (menu_item_ + 1) % kMenuItemCount;
  } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return) {
    if (menu_item_ == kConnect) {
      connection_future_.reset(
          new FutureWrapper{connect(connection_local_, connection_params_, locator_params_)});
    } else if (menu_item_ == kToggleFullscreen) {
      return ModeAction::kToggleFullscreen;
    } else if (menu_item_ == kExitApplication) {
      return ModeAction::kExitApplication;
    }
  }
  return ModeAction::kNone;
}

std::unique_ptr<Mode> TitleMode::update() {
  ++frame_;
  if (connection_future_ && connection_future_->future.Wait({0})) {
    return std::unique_ptr<Mode>{new ConnectMode{connection_future_->future.Get()}};
  }
  return {};
}

void TitleMode::render(const Renderer& renderer) const {
  auto dimensions = renderer.framebuffer_dimensions();
  auto max_texture_scale = (dimensions - glm::vec2{60.f, 180.f}) / title_.dimensions;
  float texture_scale = std::min(max_texture_scale.x, max_texture_scale.y);
  auto scaled_dimensions = texture_scale * title_.dimensions;
  auto border = (dimensions - scaled_dimensions) / glm::vec2{2.f, 6.f};
  border.y = std::min(border.y, border.x);

  {
    auto program = title_program_.use();
    glUniform1f(program.uniform("frame"), frame_);
    glUniform1f(program.uniform("random_seed"), static_cast<float>(random_seed_));
    glUniform1f(program.uniform("title_alpha"), connection_future_ ? 0.f : title_alpha(frame_));
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    glUniform2fv(program.uniform("scaled_dimensions"), 1, glm::value_ptr(scaled_dimensions));
    glUniform2fv(program.uniform("border"), 1, glm::value_ptr(border));
    program.uniform_texture("title_texture", title_.texture);
    renderer.set_simplex3_uniforms(program);
    renderer.draw_quad();
  }

  auto menu_height = dimensions.y -
      (dimensions.y - border.y - scaled_dimensions.y + kMenuItemCount * shaders::text_height) / 2;
  auto draw_menu_item = [&](const std::string& text, std::int32_t item) {
    auto text_width = renderer.text_width(text);
    renderer.draw_text(
        text, {dimensions.x / 2 - text_width / 2, menu_height + item * shaders::text_height},
        item == menu_item_ ? glm::vec4{.75f, .75f, .75f, text_alpha(frame_)}
                           : glm::vec4{.75f, .75f, .75f, .25f * text_alpha(frame_)});
  };

  if (!connection_future_) {
    draw_menu_item("CONNECT", kConnect);
    draw_menu_item("TOGGLE FULLSCREEN", kToggleFullscreen);
    draw_menu_item("EXIT", kExitApplication);
  }

  if (connection_future_) {
    auto text = "CONNECTING...";
    auto text_width = renderer.text_width(text);
    renderer.draw_text(text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, .25f});
  }
}

}  // ::gloam
