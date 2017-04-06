#include "workers/client/src/world/world.h"
#include "common/src/common/conversions.h"
#include "common/src/common/timing.h"
#include "workers/client/src/input.h"
#include <glm/glm.hpp>
#include <schema/common.h>
#include <schema/player.h>

namespace gloam {
namespace world {

World::World(worker::Connection& connection, worker::Dispatcher& dispatcher,
             const ModeState& mode_state)
: connection_{connection}, dispatcher_{dispatcher}, player_id_{-1}, world_renderer_{mode_state} {
  dispatcher_.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection_
        .SendInterestedComponents<schema::CanonicalPosition,
                                  /* TODO: replace with common component. */ schema::PlayerClient,
                                  schema::Chunk>(op.EntityId);
  });

  dispatcher_.OnAuthorityChange<schema::PlayerClient>([&](const worker::AuthorityChangeOp& op) {
    if (op.HasAuthority) {
      connection_.SendInterestedComponents<schema::CanonicalPosition, schema::PlayerClient,
                                           schema::PlayerServer>(op.EntityId);
    }
  });

  dispatcher_.OnAddComponent<schema::CanonicalPosition>(
      [&](const worker::AddComponentOp<schema::CanonicalPosition>& op) {
        entity_positions_.erase(op.EntityId);
        entity_positions_.insert(std::make_pair(op.EntityId, common::coords(op.Data.coords())));
      });

  dispatcher_.OnRemoveComponent<schema::CanonicalPosition>(
      [&](const worker::RemoveComponentOp& op) { entity_positions_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::CanonicalPosition>([&](
      const worker::ComponentUpdateOp<schema::CanonicalPosition>& op) {
    if (op.EntityId != player_id_ && op.Update.coords()) {
      entity_positions_.erase(op.EntityId);
      entity_positions_.insert(std::make_pair(op.EntityId, common::coords(*op.Update.coords())));
    }
  });

  dispatcher_.OnAddComponent<schema::PlayerClient>(
      [&](const worker::AddComponentOp<schema::PlayerClient>& op) {
        player_entities_.insert(op.EntityId);
      });

  dispatcher_.OnRemoveComponent<schema::PlayerClient>(
      [&](const worker::RemoveComponentOp& op) { player_entities_.erase(op.EntityId); });

  tile_map_.register_callbacks(dispatcher_);
}

void World::set_player_id(worker::EntityId player_id) {
  player_id_ = player_id;
}

void World::tick(const Input& input) {
  if (tile_map_.has_changed()) {
    collision_.update(tile_map_);
  }

  glm::vec2 direction;
  if (input.held(Button::kLeft)) {
    direction += glm::vec2{-1.f, -1.f};
  }
  if (input.held(Button::kRight)) {
    direction += glm::vec2{1.f, 1.f};
  }
  if (input.held(Button::kDown)) {
    direction += glm::vec2{1.f, -1.f};
  }
  if (input.held(Button::kUp)) {
    direction += glm::vec2{-1.f, 1.f};
  }
  if (direction != glm::vec2{}) {
    direction = glm::normalize(direction);
    core::Box box{1.f / 8};
    auto& position = entity_positions_[player_id_];

    auto speed_per_tick = common::kPlayerSpeed / common::kTicksPerSync;
    auto projection_xz = collision_.project_xz(box, position, speed_per_tick * direction);
    position += glm::vec3{projection_xz.x, 0.f, projection_xz.y};
    player_tick_dv_ += projection_xz / common::kPlayerSpeed;
  }
}

void World::sync() {
  ++sync_tick_;
  connection_.SendComponentUpdate<schema::PlayerClient>(
      player_id_, schema::PlayerClient::Update{}.add_sync_input(
                      {sync_tick_, player_tick_dv_.x, player_tick_dv_.y}));
  player_tick_dv_ = {};
}

void World::render(const Renderer& renderer, std::uint64_t frame) const {
  auto it = entity_positions_.find(player_id_);
  if (it == entity_positions_.end()) {
    return;
  }
  std::vector<Light> lights;
  std::vector<glm::vec3> positions;

  lights.push_back({it->second + glm::vec3{0.f, 1.f, 0.f}, 2.f, 2.f});
  for (worker::EntityId entity_id : player_entities_) {
    auto jt = entity_positions_.find(entity_id);
    positions.push_back(jt->second);
    if (entity_id != player_id_ && jt != entity_positions_.end()) {
      lights.push_back({jt->second + glm::vec3{0.f, 1.f, 0.f}, 2.f, 2.f});
    }
  }

  world_renderer_.render(renderer, frame, it->second, lights, positions, tile_map_.get());
}

}  // ::world
}  // ::gloam
