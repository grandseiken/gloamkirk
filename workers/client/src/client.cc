#include "workers/client/src/input.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/title_mode.h"
#include <SFML/Graphics.hpp>
#include <improbable/worker.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <string>

namespace {
const std::string kProjectName = "alpha_zebra_pizza_956";
const std::string kTitle = "Gloamkirk";
const std::string kWorkerType = "client";
constexpr const std::uint64_t kFramesPerSecond = 60;
constexpr const auto kMillisPerFrame = static_cast<std::uint32_t>(1000.f / kFramesPerSecond);

std::unique_ptr<sf::RenderWindow> create_window(bool fullscreen) {
  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 4;
  settings.minorVersion = 0;
  settings.attributeFlags = sf::ContextSettings::Core;

  std::unique_ptr<sf::RenderWindow> window{
      fullscreen
          ? new sf::RenderWindow{sf::VideoMode::getDesktopMode(), kTitle, sf::Style::None, settings}
          : new sf::RenderWindow{
                sf::VideoMode(gloam::native_resolution.x, gloam::native_resolution.y), kTitle,
                sf::Style::Default, settings}};
  // Note: vsync implicitly limits framerate to 60.
  window->setVerticalSyncEnabled(true);
  window->setFramerateLimit(kFramesPerSecond);
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

void run(gloam::ModeState& mode_state, bool fade_in) {
  const bool fullscreen = mode_state.fullscreen;
  auto window = create_window(fullscreen);
  glo::Init();
  gloam::Input input{reinterpret_cast<std::size_t>(window->getSystemHandle())};
  gloam::Renderer renderer;
  renderer.resize({window->getSize().x, window->getSize().y});

  auto make_title = [&](bool fade_in) {
    return new gloam::TitleMode{mode_state, fade_in, connection_params(mode_state.connect_local),
                                locator_params(mode_state.login_token)};
  };
  std::unique_ptr<gloam::Mode> mode{make_title(fade_in)};

  sf::Clock frame_clock;
  std::int32_t frames_behind = 0;
  std::uint32_t millis_behind = 0;
  while (window->isOpen()) {
    millis_behind += frame_clock.getElapsedTime().asMilliseconds();
    frame_clock.restart();
    while (millis_behind >= kMillisPerFrame) {
      millis_behind -= kMillisPerFrame;
      ++frames_behind;
    }

    do {
      --frames_behind;
      input.update();
      sf::Event event;
      while (window->pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
          mode_state.exit_application = true;
          return;
        } else if (event.type == sf::Event::Resized) {
          renderer.resize({window->getSize().x, window->getSize().y});
        } else {
          input.handle(event);
        }
      }
      mode->update(input);
    } while (!mode_state.new_mode && frames_behind > 0);

    renderer.begin_frame();
    renderer.set_default_render_states();
    mode->render(renderer);
    renderer.end_frame();
    window->display();

    if (mode_state.exit_application || mode_state.fullscreen != fullscreen) {
      return;
    } else if (mode_state.exit_to_title) {
      mode_state.exit_to_title = false;
      mode.reset(make_title(true));
    } else if (mode_state.new_mode) {
      mode.swap(mode_state.new_mode);
      mode_state.new_mode.reset();
    }
  }
}

}  // anonymous

int main(int argc, const char** argv) {
  gloam::ModeState mode_state;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--local" || arg == "-l") {
      std::cout << "[warning] Connecting to local deployment." << std::endl;
      mode_state.connect_local = true;
    } else if (arg == "--login_token" || arg == "-t") {
      mode_state.login_token = argv[++i];
    } else {
      std::cerr << "[warning] Unknown flag '" << argv[i] << "'." << std::endl;
    }
  }
  if (!mode_state.connect_local && mode_state.login_token.empty()) {
    std::cerr << "[warning] Connecting to cloud deployment, but no --login_token provided."
              << std::endl;
  }

  auto now = std::chrono::system_clock::now().time_since_epoch();
  std::mt19937 generator{static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count())};
  std::uniform_int_distribution<std::int32_t> distribution(0, 1 << 16);

  mode_state.random_seed = distribution(generator);
  mode_state.frame = 0;

  bool fade_in = true;
  while (true) {
    run(mode_state, fade_in);
    if (mode_state.exit_application) {
      break;
    }
    fade_in = false;
  }
  return 0;
}
