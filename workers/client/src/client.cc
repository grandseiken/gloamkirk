#include "common/src/common/timing.h"
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
#include <thread>

namespace {
const std::string kProjectName = "alpha_zebra_pizza_956";
const std::string kTitle = "Gloamkirk";
const std::string kWorkerType = "client";

std::unique_ptr<sf::RenderWindow> create_window(const gloam::ModeState& mode_state) {
  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 4;
  settings.minorVersion = 0;
  settings.attributeFlags = sf::ContextSettings::Core;

  std::unique_ptr<sf::RenderWindow> window{
      mode_state.fullscreen
          ? new sf::RenderWindow{sf::VideoMode::getDesktopMode(), kTitle, sf::Style::None, settings}
          : new sf::RenderWindow{sf::VideoMode(gloam::max_resolution.x, gloam::max_resolution.y),
                                 kTitle, sf::Style::Default, settings}};
  window->setFramerateLimit(0);
  window->setVerticalSyncEnabled(true);
  window->setVisible(true);
  window->display();
  return window;
}

worker::ConnectionParameters connection_params(const gloam::ModeState& mode_state) {
  worker::ConnectionParameters params = {};
  params.WorkerType = kWorkerType;
  params.Network.UseExternalIp = !mode_state.connect_local;
  // TODO: use RakNet again when connection issue is fixed.
  params.Network.ConnectionType = worker::NetworkConnectionType::kTcp;
  params.Network.RakNet.HeartbeatTimeoutMillis = 16000;
  params.Network.Tcp.NoDelay = true;
  params.ProtocolLogging.MaxLogFiles = 8;
  params.ProtocolLogging.MaxLogFileSizeBytes = 1 << 20;
  params.ProtocolLogging.LogPrefix = mode_state.worker_id + "-";
  params.EnableProtocolLoggingAtStartup = mode_state.enable_protocol_logging;
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
  auto window = create_window(mode_state);

  glo::Init();
  gloam::Input input{reinterpret_cast<std::size_t>(window->getSystemHandle())};
  gloam::Renderer renderer;
  renderer.resize({window->getSize().x, window->getSize().y});

  auto make_title = [&](bool fade_in) {
    return new gloam::TitleMode{mode_state, fade_in, connection_params(mode_state),
                                locator_params(mode_state.login_token)};
  };
  std::unique_ptr<gloam::Mode> mode{make_title(fade_in)};

  bool running = true;

  auto tick = [&](bool sync_tick, bool render_tick) {
    input.update();
    sf::Event event;
    while (window->pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        mode_state.exit_application = true;
      } else if (event.type == sf::Event::Resized) {
        renderer.resize({event.size.width, event.size.height});
      } else {
        input.handle(event);
      }
    }

    ++mode_state.frame;
    mode->tick(input);
    if (sync_tick) {
      mode->sync();
    }

    if (mode_state.exit_application || mode_state.fullscreen != fullscreen) {
      running = false;
      return;
    } else if (mode_state.exit_to_title) {
      mode_state.exit_to_title = false;
      mode.reset(make_title(true));
    } else if (mode_state.new_mode) {
      mode.swap(mode_state.new_mode);
      mode_state.new_mode.reset();
    }

    if (render_tick) {
      renderer.begin_frame();
      mode->render(renderer);
      renderer.end_frame();
    }
  };

  std::uint32_t sync = 0;
  std::uint32_t render = 0;
  auto advance_tick = [&] {
    sync = (1 + sync) % gloam::common::kTicksPerSync;
    render = (1 + render) % (mode_state.framerate == gloam::Framerate::k30Fps ? 2 : 1);
  };

  window->setActive(true);
  window->display();
  auto next_update = std::chrono::steady_clock::now();
  while (running) {
    auto start_point = std::chrono::steady_clock::now();
    bool sync_tick = !sync;
    bool render_tick = !render;
    {
      while (start_point > next_update &&
             start_point - next_update >= gloam::common::kTickDuration) {
        tick(false, false);
        advance_tick();
        next_update += gloam::common::kTickDuration;
      }
      tick(sync_tick, render_tick);

      if (render_tick) {
        window->display();
      }
    }
    if (render_tick && sync_tick) {
      auto update_time = std::chrono::steady_clock::now() - start_point;
      mode_state.client_load =
          static_cast<float>(
              std::chrono::duration_cast<std::chrono::microseconds>(update_time).count()) /
          std::chrono::duration_cast<std::chrono::microseconds>(gloam::common::kTickDuration)
              .count();
    }

    next_update += gloam::common::kTickDuration;
    if (next_update > std::chrono::steady_clock::now()) {
      std::this_thread::sleep_until(next_update);
    }
    advance_tick();
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
    } else if (arg == "--enable_protocol_logging") {
      std::cout << "[warning] Enabling protocol logging." << std::endl;
      mode_state.enable_protocol_logging = true;
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
  std::uniform_int_distribution<std::int32_t> distribution{0, 1 << 16};

  mode_state.random_seed = distribution(generator);
  mode_state.worker_id = kWorkerType + "-" + std::to_string(mode_state.random_seed);
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
