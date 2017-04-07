#include "common/src/common/conversions.h"
#include "common/src/common/timing.h"
#include "common/src/core/collision.h"
#include "common/src/core/tile_map.h"
#include "common/src/managed/managed.h"
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <improbable/worker.h>
#include <schema/common.h>
#include <schema/player.h>
#include <cstdint>
#include <unordered_map>

namespace gloam {
namespace ambient {
namespace {
const std::string kWorkerType = "ambient";
const std::uint32_t kSyncBufferCount = 8;

class PositionLogic : public managed::WorkerLogic {
public:
  void init(managed::ManagedConnection& c) override {
    c_ = &c;

    c.dispatcher.OnAddEntity([&](const worker::AddEntityOp& op) {
      c.connection.SendComponentInterest(op.EntityId,
                                         {
                                             {schema::PlayerClient::ComponentId, {true}},
                                             {schema::PlayerServer::ComponentId, {true}},
                                         });
    });

    c.dispatcher.OnRemoveEntity(
        [&](const worker::RemoveEntityOp& op) { entity_positions_.erase(op.EntityId); });

    c.dispatcher.OnAddComponent<schema::CanonicalPosition>(
        [&](const worker::AddComponentOp<schema::CanonicalPosition>& op) {
          // We only get the position when we're authoritative.
          auto& position = entity_positions_[op.EntityId];
          position.last = position.current = common::coords(op.Data.coords());
        });

    c.dispatcher.OnComponentUpdate<schema::PlayerServer>(
        [&](const worker::ComponentUpdateOp<schema::PlayerServer>& op) {
          auto& position = entity_positions_[op.EntityId];
          if (!position.has_authority && !op.Update.sync_state().empty()) {
            auto& sync = op.Update.sync_state().front();

            // Cosimulation.
            position.last = position.current;
            position.current = {sync.x(), sync.y(), sync.z()};
          }
        });

    c.dispatcher.OnAuthorityChange<schema::CanonicalPosition>(
        [&](const worker::AuthorityChangeOp& op) {
          entity_positions_[op.EntityId].has_authority = op.HasAuthority;
        });

    c.dispatcher.OnComponentUpdate<schema::PlayerClient>(
        [&](const worker::ComponentUpdateOp<schema::PlayerClient>& op) {
          auto& position = entity_positions_[op.EntityId];
          if (!op.Update.sync_input().empty()) {
            const auto& input = op.Update.sync_input().front();

            position.player_tick = input.sync_tick();
            position.xz_dv = {input.dx(), input.dz()};
            if (glm::dot(position.xz_dv, position.xz_dv) > 1.f) {
              position.xz_dv = glm::normalize(position.xz_dv);
            }
          }
        });

    tile_map_.register_callbacks(c.connection, c.dispatcher);
  }

  void tick() override {
    if (tile_map_.has_changed()) {
      collision_.update(tile_map_);
    }
  }

  void sync() override {
    for (auto& pair : entity_positions_) {
      auto& position = pair.second;
      if (!position.has_authority) {
        continue;
      }

      auto& current = position.current;
      if (position.xz_dv != glm::vec2{}) {
        core::Box box{1.f / 8};
        auto projection_xz =
            collision_.project_xz(box, current, common::kPlayerSpeed * position.xz_dv);
        current += glm::vec3{projection_xz.x, 0.f, projection_xz.y};
      }

      // Update the canonical position.
      if (current != position.last) {
        c_->connection.SendComponentUpdate<schema::CanonicalPosition>(
            pair.first, schema::CanonicalPosition::Update{}.set_coords(common::coords(current)));
      }
      // Bounce back to client (and cosimulators) for this player.
      c_->connection.SendComponentUpdate<schema::PlayerServer>(
          pair.first, schema::PlayerServer::Update{}.add_sync_state(
                          {position.player_tick, current.x, current.y, current.z}));
      position.last = current;
      ++position.player_tick;
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
    glm::vec2 xz_dv;
    glm::vec3 last;
    glm::vec3 current;
    std::uint32_t player_tick;
  };

  managed::ManagedConnection* c_ = nullptr;
  core::TileMap tile_map_;
  core::Collision collision_;
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
