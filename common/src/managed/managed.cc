#include "common/src/managed/managed.h"
#include "common/src/common/timing.h"
#include <improbable/worker.h>
#include <chrono>
#include <iostream>
#include <thread>

namespace gloam {
namespace managed {
namespace {

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
    connection_.SendLogMessage(worker::LogLevel::kInfo, logger_name_, message);
  }

  void warn(const std::string& message) const override {
    connection_.SendLogMessage(worker::LogLevel::kWarn, logger_name_, message);
  }

  void error(const std::string& message) const override {
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

  std::uint32_t sync = 0;
  auto next_update = std::chrono::steady_clock::now();
  if (connection.IsConnected()) {
    for (const auto& worker_logic : logic) {
      worker_logic->init(managed_connection);
    }
  }
  while (connected) {
    dispatcher.Process(connection.GetOpList(/* millis */ 0));
    if (!connected) {
      break;
    }
    for (const auto& worker_logic : logic) {
      auto start_point = std::chrono::steady_clock::now();
      worker_logic->tick();
      if (!sync) {
        worker_logic->sync();
        auto update_time = std::chrono::steady_clock::now() - start_point;
        auto load =
            static_cast<float>(
                std::chrono::duration_cast<std::chrono::microseconds>(update_time).count()) /
            std::chrono::duration_cast<std::chrono::microseconds>(common::kTickDuration).count();
        worker::Metrics load_report;
        load_report.Load = load;
        connection.SendMetrics(load_report);
      }
    }

    next_update += common::kTickDuration;
    std::this_thread::sleep_until(next_update);
    sync = (1 + sync) % common::kTicksPerSync;
  }
  return 1;
}

}  // ::managed
}  // ::gloam
