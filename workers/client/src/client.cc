#include "workers/client/src/renderer.h"
#include "workers/client/src/title_mode.h"
#include <SFML/Graphics.hpp>
#include <glm/common.hpp>
#include <improbable/worker.h>
#include <schema/gloamkirk.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace {
const std::string kTitle = "Gloamkirk";
const std::string kProjectName = "alpha_zebra_pizza_956";
const std::string kWorkerType = "client";
const std::string kLocalhost = "127.0.0.1";
const std::uint16_t kPort = 7777;

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

worker::Connection connect() {
  worker::ConnectionParameters params;
  params.WorkerId = kWorkerType;
  params.WorkerType = kWorkerType;
  params.Network.UseExternalIp = true;
  params.Network.ConnectionType = worker::NetworkConnectionType::kRaknet;

  std::cout << "connecting..." << std::endl;
  auto connection = worker::Connection::ConnectAsync(kLocalhost, kPort, params).Get();
  if (connection.IsConnected()) {
    std::cout << "connected!" << std::endl;
  }
  return connection;
}

gloam::ModeAction run(bool fullscreen, bool first_run) {
  auto window = create_window(fullscreen);
  glo::Init();
  gloam::Renderer renderer;
  renderer.resize({window->getSize().x, window->getSize().y});

  auto connection = connect();

  bool connected = true;
  worker::Dispatcher dispatcher;
  dispatcher.OnDisconnect([&connected](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected = false;
  });

  dispatcher.OnLogMessage([&connected](const worker::LogMessageOp& op) {
    switch (op.Level) {
    case worker::LogLevel::kDebug:
      std::cout << "[debug] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kInfo:
      std::cout << "[info] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kWarn:
      std::cerr << "[warning] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kError:
      std::cerr << "[error] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kFatal:
      std::cerr << "[fatal] " << op.Message << std::endl;
      connected = false;
    default:
      break;
    }
  });

  dispatcher.OnMetrics([&connection](const worker::MetricsOp& op) {
    auto metrics = op.Metrics;
    connection.SendMetrics(metrics);
  });

  std::unique_ptr<gloam::Mode> mode{new gloam::TitleMode{first_run}};
  while (window->isOpen()) {
    sf::Event event;
    while (window->pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        return gloam::ModeAction::kExitApplication;
      } else if (event.type == sf::Event::Resized) {
        renderer.resize({window->getSize().x, window->getSize().y});
      } else {
        auto mode_action = mode->event(event);
        if (mode_action == gloam::ModeAction::kExitApplication ||
            mode_action == gloam::ModeAction::kToggleFullscreen) {
          return mode_action;
        }
      }
    }

    if (connected) {
      dispatcher.Process(connection.GetOpList(/* millis */ 16));
    }

    renderer.begin_frame();
    renderer.set_default_render_states();
    mode->update();
    mode->render(renderer);
    renderer.end_frame();
    window->display();
  }
  return gloam::ModeAction::kExitApplication;
}

}  // anonymous

int main(int, const char**) {
  bool fullscreen = false;
  bool first_run = true;
  while (true) {
    auto mode_action = run(fullscreen, first_run);
    if (mode_action == gloam::ModeAction::kToggleFullscreen) {
      fullscreen = !fullscreen;
    } else if (mode_action == gloam::ModeAction::kExitApplication) {
      break;
    }
    first_run = false;
  }
  return 0;
}
