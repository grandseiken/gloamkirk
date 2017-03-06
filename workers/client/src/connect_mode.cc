#include "workers/client/src/connect_mode.h"
#include <iostream>

namespace gloam {

ConnectMode::ConnectMode(worker::Connection&& connection) : connection_{std::move(connection)} {
  dispatcher_.OnDisconnect([this](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected_ = false;
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
    connection_.SendMetrics(metrics);
  });
}

ModeAction ConnectMode::event(const sf::Event&) {
  return ModeAction::kNone;
}

std::unique_ptr<Mode> ConnectMode::update() {
  if (connected_) {
    dispatcher_.Process(connection_.GetOpList(/* millis */ 4));
  }
  return {};
}

void ConnectMode::render(const Renderer&) const {}

}  // ::gloam
