#include "workers/client/src/input.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/title_mode.h"
#include <SFML/Graphics.hpp>
#include <improbable/worker.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace {
const std::string kProjectName = "alpha_zebra_pizza_956";
const std::string kTitle = "Gloamkirk";
const std::string kWorkerType = "client";

std::unique_ptr<sf::RenderWindow> create_window(bool fullscreen) {
  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 3;
  settings.minorVersion = 3;
  settings.attributeFlags = sf::ContextSettings::Core;

  std::unique_ptr<sf::RenderWindow> window{
      fullscreen
          ? new sf::RenderWindow{sf::VideoMode::getDesktopMode(), kTitle, sf::Style::None, settings}
          : new sf::RenderWindow{
                sf::VideoMode(gloam::native_resolution.x, gloam::native_resolution.y), kTitle,
                sf::Style::Default, settings}};
  // Note: vsync implicitly limits framerate to 60.
  window->setVerticalSyncEnabled(true);
  window->setFramerateLimit(60);
  window->setVisible(true);
  window->display();
  return window;
}

worker::ConnectionParameters connection_params(bool local) {
  auto time_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

  worker::ConnectionParameters params = {};
  params.WorkerId = kWorkerType + "-" + std::to_string(time_millis);
  params.WorkerType = kWorkerType;
  params.Network.UseExternalIp = !local;
  params.Network.ConnectionType = worker::NetworkConnectionType::kRaknet;
  params.Network.RakNet.HeartbeatTimeoutMillis = 16000;
  return params;
}

worker::LocatorParameters locator_params(const std::string& login_token) {
  worker::LocatorParameters params = {};
  params.ProjectName = kProjectName;
  params.CredentialsType = worker::LocatorCredentialsType::kLoginToken;
  params.LoginToken.Token = login_token;
  return params;
}

gloam::ModeAction run(bool fullscreen, bool first_run, bool local, const std::string& login_token) {
  auto window = create_window(fullscreen);
  glo::Init();
  gloam::Input input{reinterpret_cast<std::size_t>(window->getSystemHandle())};
  gloam::Renderer renderer;
  renderer.resize({window->getSize().x, window->getSize().y});

  auto make_title = [&](bool fade_in) {
    auto title_mode = new gloam::TitleMode{first_run, fade_in, local, connection_params(local),
                                           locator_params(login_token)};
    return title_mode;
  };
  std::unique_ptr<gloam::Mode> mode{make_title(first_run)};
  first_run = false;

  while (window->isOpen()) {
    input.update();
    sf::Event event;
    while (window->pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        return gloam::ModeAction::kExitApplication;
      } else if (event.type == sf::Event::Resized) {
        renderer.resize({window->getSize().x, window->getSize().y});
      } else {
        input.handle(event);
      }
    }

    // TODO: need to drop frames when rendering too slowly!
    renderer.begin_frame();
    renderer.set_default_render_states();
    auto mode_result = mode->update(input);
    mode->render(renderer);
    renderer.end_frame();
    window->display();

    if (mode_result.action == gloam::ModeAction::kExitApplication ||
        mode_result.action == gloam::ModeAction::kToggleFullscreen) {
      return mode_result.action;
    } else if (mode_result.action == gloam::ModeAction::kExitToTitle) {
      mode.reset(make_title(true));
    } else if (mode_result.new_mode) {
      mode.swap(mode_result.new_mode);
    }
  }
  return gloam::ModeAction::kExitApplication;
}

}  // anonymous

int main(int argc, const char** argv) {
  bool local = false;
  std::string login_token;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--local" || arg == "-l") {
      std::cout << "[warning] Connecting to local deployment." << std::endl;
      local = true;
    } else if (arg == "--login_token" || arg == "-t") {
      login_token = argv[++i];
    } else {
      std::cerr << "[warning] Unknown flag '" << argv[i] << "'." << std::endl;
    }
  }
  if (!local && login_token.empty()) {
    std::cerr << "[warning] Connecting to cloud deployment, but no --login_token provided."
              << std::endl;
  }

  bool fullscreen = false;
  bool first_run = true;
  while (true) {
    auto mode_action = run(fullscreen, first_run, local, login_token);
    if (mode_action == gloam::ModeAction::kToggleFullscreen) {
      fullscreen = !fullscreen;
    } else if (mode_action == gloam::ModeAction::kExitApplication) {
      break;
    }
    first_run = false;
  }
  return 0;
}
