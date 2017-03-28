#include "common/src/common/conversions.h"
#include "common/src/common/timing.h"
#include "common/src/managed/managed.h"
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <improbable/worker.h>
#include <schema/common.h>
#include <schema/player.h>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace gloam {
namespace ambient {
namespace {
const std::string kWorkerType = "ambient";
const std::uint32_t kSyncBufferCount = 8;

class PositionLogic : public managed::WorkerLogic {
public:
  void init(managed::ManagedConnection& c) override {
    c_ = &c;

    c.dispatcher.OnAddComponent<schema::CanonicalPosition>(
        [&](const worker::AddComponentOp<schema::CanonicalPosition>& op) {
          auto& position = entity_positions_[op.EntityId];
          position.missed_syncs = 0;
          position.current = common::coords(op.Data.coords());
          c.connection.SendInterestedComponents<schema::Player>(op.EntityId);
        });

    c.dispatcher.OnComponentUpdate<schema::CanonicalPosition>(
        [&](const worker::ComponentUpdateOp<schema::CanonicalPosition>& op) {
          auto it = entity_positions_.find(op.EntityId);
          if (it != entity_positions_.end() && !it->second.has_authority && op.Update.coords()) {
            // Cosimulation.
            it->second.current = common::coords(*op.Update.coords());
          }
        });

    c.dispatcher.OnRemoveComponent<schema::CanonicalPosition>(
        [&](const worker::RemoveComponentOp& op) { entity_positions_.erase(op.EntityId); });

    c.dispatcher.OnAuthorityChange<schema::CanonicalPosition>(
        [&](const worker::AuthorityChangeOp& op) {
          entity_positions_[op.EntityId].has_authority = op.HasAuthority;
        });

    c.dispatcher.OnComponentUpdate<schema::Player>(
        [&](const worker::ComponentUpdateOp<schema::Player>& op) {
          auto it = entity_positions_.find(op.EntityId);
          if (it == entity_positions_.end() || !op.Update.sync_tick()) {
            return;
          }
          PositionSync sync;
          sync.tick = *op.Update.sync_tick();
          sync.xz = {op.Update.dx().value_or(0), op.Update.dz().value_or(0)};
          if (glm::dot(sync.xz, sync.xz) > 1.f) {
            sync.xz = glm::normalize(sync.xz);
          }

          it->second.ticks.push_back(sync);
          if (it->second.ticks.size() > kSyncBufferCount) {
            it->second.ticks.pop_front();
          }
        });
  }

  void tick() override {}
  void sync() override {
    for (auto& pair : entity_positions_) {
      pair.second.missed_syncs = std::max(kSyncBufferCount, 1 + pair.second.missed_syncs);
      if (pair.second.ticks.empty()) {
        continue;
      }

      while (pair.second.missed_syncs && !pair.second.ticks.empty()) {
        const auto& tick = pair.second.ticks.front();
        pair.second.current += common::kPlayerSpeed * glm::vec3{tick.xz.x, 0.f, tick.xz.y};
        pair.second.ticks.pop_front();
        --pair.second.missed_syncs;
      }
      c_->connection.SendComponentUpdate<schema::CanonicalPosition>(
          pair.first,
          schema::CanonicalPosition::Update{}.set_coords(common::coords(pair.second.current)));
    }
  }

private:
  struct PositionSync {
    std::uint32_t tick = 0;
    // Normalized xz direction.
    glm::vec2 xz;
  };

  struct Position {
    bool has_authority = false;
    glm::vec3 current;

    std::uint32_t missed_syncs = 0;
    std::deque<PositionSync> ticks;
  };

  managed::ManagedConnection* c_ = nullptr;
  std::unordered_map<worker::EntityId, Position> entity_positions_;
};

}  // anonymous
}  // ::ambient
}  // ::gloam

int main(int argc, char** argv) {
  gloam::ambient::PositionLogic position_logic;
  std::vector<gloam::managed::WorkerLogic*> worker_logic{&position_logic};
  return gloam::managed::connect(gloam::ambient::kWorkerType, worker_logic, argc, argv);
}
