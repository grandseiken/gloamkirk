#ifndef GLOAM_WORKERS_CLIENT_SRC_WORLD_PLAYER_CONTROLLER_H
#define GLOAM_WORKERS_CLIENT_SRC_WORLD_PLAYER_CONTROLLER_H
#include "common/src/common/hashes.h"
#include "common/src/core/collision.h"
#include "common/src/core/tile_map.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/world/world_renderer.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <improbable/worker.h>
#include <schema/chunk.h>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace worker {
class Connection;
class Dispatcher;
}  // ::worker

namespace gloam {
class Input;
class Renderer;

namespace world {

class PlayerController {
public:
  PlayerController(worker::Connection& connection_, worker::Dispatcher& dispatcher_,
                   const ModeState& mode_state);
  void set_player_id(worker::EntityId player_id);

  // Game loop functions: sync() is called at the network frame-rate; update() is called at the
  // client rendering frame-rate.
  void tick(const Input& input);
  void sync();
  void render(const Renderer& renderer, std::uint64_t frame) const;

private:
  void reconcile(std::uint32_t sync_tick, const glm::vec3& coordinates);

  worker::Connection& connection_;
  worker::Dispatcher& dispatcher_;

  // Player status.
  worker::EntityId player_id_;
  glm::vec3 canonical_position_;
  std::uint32_t sync_tick_ = 0;
  glm::vec2 player_tick_dv_;

  // History for reconciliation.
  struct InputHistory {
    std::uint32_t sync_tick;
    glm::vec2 xz_dv;
  };
  std::deque<InputHistory> input_history_;

  // Other entity data.
  std::unordered_set<worker::EntityId> player_entities_;
  std::unordered_map<worker::EntityId, glm::vec3> entity_positions_;

  core::TileMap tile_map_;
  core::Collision collision_;
  WorldRenderer world_renderer_;
};

}  // ::world
}  // ::gloam

#endif
