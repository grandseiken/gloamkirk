#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/title.h"
#include <SFML/Graphics.hpp>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <improbable/worker.h>
#include <schema/gloamkirk.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <string>

namespace {
const std::string kTitle = "Gloamkirk";
const std::string kProjectName = "alpha_zebra_pizza_956";
const std::string kWorkerType = "client";
const std::string kLocalhost = "127.0.0.1";
const std::uint16_t kPort = 7777;

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

struct TextureImage {
  glo::Texture texture;
  glm::vec2 dimensions;
};

TextureImage load_texture(const std::string& path) {
  sf::Image image;
  image.loadFromFile(path);

  TextureImage result;
  result.dimensions = {image.getSize().x, image.getSize().y};
  result.texture.create_2d(result.dimensions, 4, GL_UNSIGNED_BYTE, image.getPixelsPtr());
  return result;
}

}  // anonymous

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

  auto title = load_texture("assets/title.png");
  title.texture.set_linear();

  glo::Program title_program{
      "title",
      {"title_vertex", GL_VERTEX_SHADER, gloam::shaders::title_vertex},
      {"title_fragment", GL_FRAGMENT_SHADER, gloam::shaders::title_fragment}};

  auto now = std::chrono::system_clock::now().time_since_epoch();
  std::mt19937 generator{static_cast<unsigned int>(
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count())};
  std::uniform_int_distribution<std::int32_t> distribution(0, 1 << 16);
  std::uint64_t frame = 0;
  std::int32_t random_seed = distribution(generator);

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

    renderer.begin_frame();
    {
      auto program = title_program.use();
      glUniform1f(program.uniform("frame"), static_cast<float>(++frame));
      glUniform1f(program.uniform("random_seed"), static_cast<float>(random_seed));
      glUniform2fv(program.uniform("dimensions"), 1,
                   glm::value_ptr(renderer.framebuffer_dimensions()));
      glUniform2fv(program.uniform("title_dimensions"), 1, glm::value_ptr(title.dimensions));
      program.uniform_texture("title_texture", title.texture);
      renderer.set_simplex3_uniforms(program);
      renderer.draw_quad();
    }
    renderer.end_frame();
    window->display();
  }
}