#include "workers/client/src/world/world.h"
#include "common/src/common/conversions.h"
#include "workers/client/src/input.h"
#include <glm/glm.hpp>
#include <schema/common.h>

namespace gloam {
namespace world {

World::World(worker::Connection& connection, worker::Dispatcher& dispatcher)
: connection_{connection}, dispatcher_{dispatcher}, player_id_{-1} {
  dispatcher_.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection_.SendInterestedComponents<schema::Position, schema::Chunk>(op.EntityId);
  });

  dispatcher_.OnAddComponent<schema::Position>(
      [&](const worker::AddComponentOp<schema::Position>& op) {
        entity_positions_.insert(std::make_pair(op.EntityId, common::coords(op.Data.coords())));
      });

  dispatcher_.OnRemoveComponent<schema::Position>(
      [&](const worker::RemoveComponentOp& op) { entity_positions_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::Position>([&](
      const worker::ComponentUpdateOp<schema::Position>& op) {
    if (op.Update.coords()) {
      entity_positions_.insert(std::make_pair(op.EntityId, common::coords(*op.Update.coords())));
    }
  });

  dispatcher_.OnAddComponent<schema::Chunk>([&](const worker::AddComponentOp<schema::Chunk>& op) {
    chunk_map_.emplace(op.EntityId, op.Data);
    update_chunk(op.Data);
  });

  dispatcher_.OnRemoveComponent<schema::Chunk>([&](const worker::RemoveComponentOp& op) {
    auto it = chunk_map_.find(op.EntityId);
    if (it != chunk_map_.end()) {
      clear_chunk(it->second);
      chunk_map_.erase(op.EntityId);
    }
  });

  dispatcher_.OnComponentUpdate<schema::Chunk>(
      [&](const worker::ComponentUpdateOp<schema::Chunk>& op) {
        auto it = chunk_map_.find(op.EntityId);
        if (it != chunk_map_.end()) {
          op.Update.ApplyTo(it->second);
          update_chunk(it->second);
        }
      });
}

void World::set_player_id(worker::EntityId player_id) {
  player_id_ = player_id;
}

void World::update(const Input& input) {
  glm::vec3 direction;
  if (input.held(Button::kLeft)) {
    direction += glm::vec3{-1.f, 0.f, -1.f};
  }
  if (input.held(Button::kRight)) {
    direction += glm::vec3{1.f, 0.f, 1.f};
  }
  if (input.held(Button::kDown)) {
    direction += glm::vec3{1.f, 0.f, -1.f};
  }
  if (input.held(Button::kUp)) {
    direction += glm::vec3{-1.f, 0.f, 1.f};
  }
  if (direction != glm::vec3{}) {
    entity_positions_[player_id_] += 1.5f * glm::normalize(direction);
  }
}

void World::render(const Renderer& renderer) const {
  auto it = entity_positions_.find(player_id_);
  if (it == entity_positions_.end()) {
    return;
  }
  world_renderer_.render(renderer, it->second, tile_map_);
}

void World::update_chunk(const schema::ChunkData& data) {
  common::update_chunk(tile_map_, data);
  collision_.update(tile_map_);
}

void World::clear_chunk(const schema::ChunkData& data) {
  common::clear_chunk(tile_map_, data);
  collision_.update(tile_map_);
}

}  // ::world
}  // ::gloam
