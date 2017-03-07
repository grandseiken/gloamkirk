#include "common/src/managed/managed.h"

namespace gloam {
namespace master {
namespace {
const std::string kWorkerType = "master";

class Worker : public gloam::managed::ManagedWorker {
public:
  void init(gloam::managed::ManagedConnection& connection) override {
    connection.logger.info("Connected.");
  }

  void update(gloam::managed::ManagedConnection&) override {}
};

}  // anonymous
}  // ::master
}  // ::gloam

int main(int argc, char** argv) {
  gloam::master::Worker worker;
  return gloam::managed::connect(gloam::master::kWorkerType, worker, argc, argv);
}
