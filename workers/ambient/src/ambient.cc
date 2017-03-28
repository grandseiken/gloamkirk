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
          c.connection.SendInterestedComponents<schema::PlayerClient>(op.EntityId);
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

    c.dispatcher.OnComponentUpdate<schema::PlayerClient>(
        [&](const worker::ComponentUpdateOp<schema::PlayerClient>& op) {
          auto it = entity_positions_.find(op.EntityId);
          if (it == entity_positions_.end() || op.Update.sync_input().empty()) {
            return;
          }
          const auto& input = op.Update.sync_input().front();
          PositionSync sync;
          sync.tick = input.sync_tick();
          sync.xz = {input.dx(), input.dz()};
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
      auto& position = pair.second;
      position.missed_syncs = std::max(kSyncBufferCount, 1 + position.missed_syncs);
      if (position.ticks.empty()) {
        continue;
      }

      std::uint32_t sync_tick = 0;
      while (position.missed_syncs && !position.ticks.empty()) {
        const auto& tick = position.ticks.front();
        sync_tick = tick.tick;
        position.current += common::kPlayerSpeed * glm::vec3{tick.xz.x, 0.f, tick.xz.y};
        position.ticks.pop_front();
        --pair.second.missed_syncs;
      }
      const auto& current = position.current;
      c_->connection.SendComponentUpdate<schema::CanonicalPosition>(
          pair.first, schema::CanonicalPosition::Update{}.set_coords(common::coords(current)));
      // Bounce back only to client for this player.
      c_->connection.SendComponentUpdate<schema::PlayerServer>(
          pair.first,
          schema::PlayerServer::Update{}.add_sync_state(
              {sync_tick, current.x, current.y, current.z}));
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
