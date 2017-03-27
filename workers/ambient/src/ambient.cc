#include "common/src/managed/managed.h"
#include <improbable/worker.h>
#include <schema/common.h>
#include <schema/player.h>
#include <unordered_set>

namespace gloam {
namespace ambient {
namespace {
const std::string kWorkerType = "ambient";

class PositionLogic : public managed::WorkerLogic {
public:
  void init(managed::ManagedConnection& c) override {
    c.dispatcher.OnAuthorityChange<schema::CanonicalPosition>(
        [&](const worker::AuthorityChangeOp& op) {
          if (op.HasAuthority) {
            c.connection.SendInterestedComponents<schema::Player>(op.EntityId);
            authority_.insert(op.EntityId);
          } else {
            authority_.erase(op.EntityId);
          }
        });

    c.dispatcher.OnComponentUpdate<schema::Player>(
        [&](const worker::ComponentUpdateOp<schema::Player>& op) {
          // TODO: temporary client-side authority; we simply mirror the client-controlled position
          // in the canonical position component.
          if (op.Update.position()) {
            c.connection.SendComponentUpdate<schema::CanonicalPosition>(
                op.EntityId, schema::CanonicalPosition::Update{}.set_coords(*op.Update.position()));
          }
        });
  }

  void update() override {}

private:
  std::unordered_set<worker::EntityId> authority_;
};

}  // anonymous
}  // ::ambient
}  // ::gloam

int main(int argc, char** argv) {
  gloam::ambient::PositionLogic position_logic;
  std::vector<gloam::managed::WorkerLogic*> worker_logic{&position_logic};
  return gloam::managed::connect(gloam::ambient::kWorkerType, worker_logic, argc, argv);
}
