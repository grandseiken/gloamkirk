#include "common/src/managed/managed.h"
#include "workers/master/src/client_handler.h"
#include "workers/master/src/master_data.h"
#include "workers/master/src/world_spawner.h"

namespace {
const std::string kWorkerType = "master";
}  // anonymous

int main(int argc, char** argv) {
  gloam::master::MasterData master_data;
  gloam::master::WorldSpawner world_spawner{master_data.data()};
  gloam::master::ClientHandler client_handler{master_data.data()};
  std::vector<gloam::managed::WorkerLogic*> worker_logic{&master_data, &world_spawner,
                                                         &client_handler};
  return gloam::managed::connect(kWorkerType, worker_logic, argc, argv);
}
