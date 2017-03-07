#include "common/src/managed/managed.h"
#include "workers/master/src/client_handler.h"

namespace {
const std::string kWorkerType = "master";
}  // anonymous

int main(int argc, char** argv) {
  gloam::master::ClientHandler client_handler;
  std::vector<gloam::managed::WorkerLogic*> worker_logic{&client_handler};
  return gloam::managed::connect(kWorkerType, worker_logic, argc, argv);
}
