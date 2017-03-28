#include "common/src/common/timing.h"
#include "workers/client/src/input.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/title_mode.h"
#include <SFML/Graphics.hpp>
#include <improbable/worker.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
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
          : new sf::RenderWindow{
                sf::VideoMode(gloam::native_resolution.x, gloam::native_resolution.y), kTitle,
                sf::Style::Default, settings}};
  window->setVerticalSyncEnabled(true);
  window->setFramerateLimit(0);
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
  auto window = create_window(mode_state);

  glo::Init();
  gloam::Input input{reinterpret_cast<std::size_t>(window->getSystemHandle())};
  gloam::Renderer renderer;
  renderer.resize({window->getSize().x, window->getSize().y});

  auto make_title = [&](bool fade_in) {
    return new gloam::TitleMode{mode_state, fade_in, connection_params(mode_state.connect_local),
                                locator_params(mode_state.login_token)};
  };
  std::unique_ptr<gloam::Mode> mode{make_title(fade_in)};

  // Synchronization data.
  std::atomic<bool> running{true};
  std::mutex render_mutex;
  std::mutex window_mutex;
  bool render_signal = false;
  std::condition_variable render_cv;
  std::deque<sf::Event> event_queue;

  window->setActive(false);

  auto tick = [&](bool sync_tick, bool render_tick) {
    input.update();
    for (const auto& event : event_queue) {
      if (event.type == sf::Event::Closed) {
        mode_state.exit_application = true;
      } else if (event.type == sf::Event::Resized) {
        renderer.resize({event.size.width, event.size.height});
      } else {
        input.handle(event);
      }
    }
    event_queue.clear();

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

  std::thread tick_thread{[&] {
    std::uint32_t sync = 0;
    std::uint32_t render = 0;
    auto next_update = std::chrono::steady_clock::now();

    while (running) {
      {
        std::unique_lock<std::mutex> lock{render_mutex};
        render_cv.wait(lock, [&] { return !render_signal || !running; });
      }
      if (!running) {
        return;
      }

      auto now = std::chrono::steady_clock::now();
      bool delayed = now - next_update >= gloam::common::kTickDuration;
      bool render_tick = !render && !delayed;
      {
        std::lock_guard<std::mutex> lock{window_mutex};
        window->setActive(true);
        tick(!sync, render_tick);
        window->setActive(false);
      }

      if (render_tick) {
        {
          std::lock_guard<std::mutex> lock{render_mutex};
          render_signal = true;
        }
        render_cv.notify_one();
      }

      next_update += sync ? gloam::common::kTickDuration : gloam::common::kSyncTickDuration;
      std::this_thread::sleep_until(next_update);
      sync = (1 + sync) % gloam::common::kTicksPerSync;
      render = (1 + render) % (mode_state.framerate == gloam::Framerate::k30Fps ? 2 : 1);
    }
  }};

  std::thread render_thread{[&] {
    while (running) {
      {
        std::unique_lock<std::mutex> lock{render_mutex};
        render_cv.wait(lock, [&] { return render_signal || !running; });
      }
      if (!running) {
        return;
      }

      // We're barely actually multithreading at all, here: only one thread is really doing
      // meaningful work at any one time. Nevertheless, running a dedicated thread just for calling
      // window->display() seems to make a lot of awkward timing issues go away.
      {
        std::lock_guard<std::mutex> lock{window_mutex};
        window->setActive(true);
        window->display();
        window->setActive(false);
      }
      {
        std::unique_lock<std::mutex> lock{render_mutex};
        render_signal = false;
      }
      render_cv.notify_one();
    }
  }};

  while (running) {
    // Process SFML window events. This must happen on the main thread.
    {
      std::lock_guard<std::mutex> lock{window_mutex};
      sf::Event event;
      while (window->pollEvent(event)) {
        event_queue.emplace_back(event);
      }
    }
    // Try to process at least once per tick.
    std::this_thread::sleep_for(gloam::common::kTickDuration / 2);
  }

  running = false;
  render_cv.notify_all();
  render_thread.join();
  tick_thread.join();
  // For OpenGL destruction!
  window->setActive(true);
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
