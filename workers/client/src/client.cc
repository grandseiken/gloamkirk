#include <improbable/worker.h>
#include <schema/gloamkirk.h>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
static const std::string kProjectName = "alpha_zebra_pizza_956";
static const std::string kWorkerType = "client";
static const std::string kLocalhost = "127.0.0.1";
static const std::uint16_t kPort = 7777;
}

int main(int, const char**) {
  worker::ConnectionParameters params;
  params.WorkerId = kWorkerType;
  params.WorkerType = kWorkerType;
  params.Network.UseExternalIp = true;
  params.Network.ConnectionType = worker::NetworkConnectionType::kRaknet;

  std::cout << "connecting" << std::endl;
  auto connection = worker::Connection::ConnectAsync(kLocalhost, kPort, params).Get();
  bool connected = true;
  worker::Dispatcher dispatcher;

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

  dispatcher.OnDisconnect([&connected](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected = false;
  });

  while (connected) {
    dispatcher.Process(connection.GetOpList(/* millis */ 16));
  }
}
