#include "common/src/managed/managed.h"
#include <improbable/worker.h>
#include <chrono>
#include <iostream>
#include <thread>

namespace gloam {
namespace managed {
namespace {
constexpr const std::uint64_t kFramesPerSecond = 20;

worker::Connection connect(const std::string& worker_type, const std::string& worker_id,
                           const std::string& hostname, std::uint16_t port) {
  worker::ConnectionParameters params = {};
  params.WorkerId = worker_id;
  params.WorkerType = worker_type;
  params.Network.UseExternalIp = false;
  params.Network.ConnectionType = worker::NetworkConnectionType::kTcp;
  params.Network.Tcp.MultiplexLevel = 8;
  params.Network.Tcp.NoDelay = true;

  return worker::Connection::ConnectAsync(hostname, port, params).Get();
}

class ManagedLoggerImpl : public ManagedLogger {
public:
  ManagedLoggerImpl(worker::Connection& connection, bool& connected, const std::string& logger_name)
  : connection_{connection}, connected_{connected}, logger_name_{logger_name} {}

  void info(const std::string& message) const override {
    std::cout << "[info] " << message << std::endl;
    connection_.SendLogMessage(worker::LogLevel::kInfo, logger_name_, message);
  }

  void warn(const std::string& message) const override {
    std::cerr << "[warn] " << message << std::endl;
    connection_.SendLogMessage(worker::LogLevel::kWarn, logger_name_, message);
  }

  void error(const std::string& message) const override {
    std::cerr << "[error] " << message << std::endl;
    connection_.SendLogMessage(worker::LogLevel::kError, logger_name_, message);
  }

  void fatal(const std::string& message) const override {
    std::cerr << "[fatal] " << message << std::endl;
    connection_.SendLogMessage(worker::LogLevel::kFatal, logger_name_, message);
    connected_ = false;
  }

private:
  worker::Connection& connection_;
  bool& connected_;
  std::string logger_name_;
};

}  // anonymous

int connect(const std::string& worker_type, const std::vector<WorkerLogic*>& logic, int argc,
            char** argv) {
  if (argc != 4) {
    std::cerr << "[error] Usage: " << argv[0] << " worker_id hostname port" << std::endl;
    return 1;
  }

  std::string worker_id = argv[1];
  std::string hostname = argv[2];
  auto port = static_cast<std::uint16_t>(std::stoul(argv[3]));

  auto connection = connect(worker_type, worker_id, hostname, port);
  bool connected = true;
  worker::Dispatcher dispatcher;

  ManagedLoggerImpl managed_logger{connection, connected, worker_type};
  ManagedConnection managed_connection{managed_logger, connection, dispatcher};

  dispatcher.OnDisconnect([&](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected = false;
  });

  dispatcher.OnLogMessage([&](const worker::LogMessageOp& op) {
    if (op.Level == worker::LogLevel::kInfo) {
      managed_logger.info(op.Message);
    } else if (op.Level == worker::LogLevel::kWarn) {
      managed_logger.warn(op.Message);
    } else if (op.Level == worker::LogLevel::kError) {
      managed_logger.error(op.Message);
    } else if (op.Level == worker::LogLevel::kFatal) {
      managed_logger.fatal(op.Message);
    }
  });

  dispatcher.OnMetrics([&](const worker::MetricsOp& op) {
    auto metrics = op.Metrics;
    connection.SendMetrics(metrics);
  });

  std::chrono::steady_clock clock;
  auto next_update = clock.now();

  for (const auto& worker_logic : logic) {
    worker_logic->init(managed_connection);
  }
  while (connected) {
    dispatcher.Process(connection.GetOpList(/* millis */ 0));
    for (const auto& worker_logic : logic) {
      worker_logic->update();
    }

    next_update += std::chrono::duration<std::uint64_t, std::ratio<1, kFramesPerSecond>>{1};
    std::this_thread::sleep_until(next_update);
  }
  return 1;
}

}  // ::managed
}  // ::gloam
