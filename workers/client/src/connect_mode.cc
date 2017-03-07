#include "workers/client/src/connect_mode.h"
#include <glm/vec4.hpp>
#include <schema/gloamkirk.h>
#include <iostream>

namespace gloam {

ConnectMode::ConnectMode(const std::string& disconnect_reason)
: connected_{false}, disconnect_reason_{disconnect_reason} {}

ConnectMode::ConnectMode(worker::Connection&& connection)
: connection_{new worker::Connection{std::move(connection)}} {
  dispatcher_.OnDisconnect([this](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected_ = false;
    disconnect_reason_ = op.Reason;
  });

  dispatcher_.OnLogMessage([this](const worker::LogMessageOp& op) {
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
      connected_ = false;
    default:
      break;
    }
  });

  dispatcher_.OnMetrics([this](const worker::MetricsOp& op) {
    auto metrics = op.Metrics;
    connection_->SendMetrics(metrics);
  });
}

ModeResult ConnectMode::event(const sf::Event& event) {
  if (!connected_ && event.type == sf::Event::KeyPressed) {
    disconnect_ack_ = true;
  }
  return {ModeAction::kNone, {}};
}

ModeResult ConnectMode::update() {
  if (connected_) {
    dispatcher_.Process(connection_->GetOpList(/* millis */ 0));
  } else if (disconnect_reason_.empty() || disconnect_ack_) {
    return {ModeAction::kExitToTitle, {}};
  }
  return {{}, {}};
}

void ConnectMode::render(const Renderer& renderer) const {
  if (!connected_) {
    auto dimensions = renderer.framebuffer_dimensions();
    auto text_width = renderer.text_width(disconnect_reason_);
    renderer.draw_text(disconnect_reason_, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, 1.f});
    return;
  }
}

}  // ::gloam
