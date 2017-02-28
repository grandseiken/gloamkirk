#include "workers/client/src/renderer.h"
#include <GL/glew.h>
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
}

std::unique_ptr<sf::RenderWindow> create_window() {
  sf::ContextSettings settings;
  settings.depthBits = 0;
  settings.stencilBits = 0;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 3;
  settings.minorVersion = 3;
  settings.attributeFlags = sf::ContextSettings::Core;

  std::unique_ptr<sf::RenderWindow> window(
      new sf::RenderWindow{sf::VideoMode::getDesktopMode(), kTitle, sf::Style::None, settings});
  window->setVerticalSyncEnabled(true);
  window->setFramerateLimit(120);
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

int main(int, const char**) {
  auto window = create_window();
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

  while (window->isOpen()) {
    sf::Event event;
    while (window->pollEvent(event)) {
      if (event.type == sf::Event::Closed ||
          (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)) {
        window->close();
      } else if (event.type == sf::Event::Resized) {
        renderer.resize({window->getSize().x, window->getSize().y});
      }
    }

    if (connected) {
      dispatcher.Process(connection.GetOpList(/* millis */ 16));
    }
    renderer.render_frame();
    window->display();
  }
}
