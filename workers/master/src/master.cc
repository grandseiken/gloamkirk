#include "common/src/managed/managed.h"
#include "workers/master/src/client_handler.h"
#include "workers/master/src/world_spawner.h"
#include <improbable/worker.h>
#include <schema/master.h>

namespace gloam {
namespace master {
namespace {
const std::string kWorkerType = "master";

class MasterData : public managed::WorkerLogic {
public:
  MasterData() : data_{false, {}} {}

  const schema::MasterData& data() {
    return data_;
  }

  void init(managed::ManagedConnection& c) override {
    c.dispatcher.OnAddComponent<schema::Master>(
        [&](const worker::AddComponentOp<schema::Master>& op) { data_ = op.Data; });

    c.dispatcher.OnComponentUpdate<schema::Master>(
        [&](const worker::ComponentUpdateOp<schema::Master>& op) { op.Update.ApplyTo(data_); });
  }

  void update() override {}

private:
  schema::MasterData data_;
};

}  // anonymous
}  // ::master
}  // ::gloam

int main(int argc, char** argv) {
  gloam::master::MasterData master_data;
  gloam::master::WorldSpawner world_spawner{master_data.data()};
  gloam::master::ClientHandler client_handler{master_data.data()};
  std::vector<gloam::managed::WorkerLogic*> worker_logic{&master_data, &world_spawner,
                                                         &client_handler};
  return gloam::managed::connect(gloam::master::kWorkerType, worker_logic, argc, argv);
}
