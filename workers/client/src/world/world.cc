#include "workers/client/src/world/world.h"
#include "workers/client/src/input.h"
#include <glm/glm.hpp>
#include <schema/common.h>

namespace gloam {
namespace world {
namespace {

glm::vec3 coords(const improbable::math::Coordinates& schema_coords) {
  return {schema_coords.x(), schema_coords.y(), schema_coords.z()};
}

glm::ivec2 tile_coords(const schema::ChunkData& data, std::size_t tile_index) {
  auto index = static_cast<std::int32_t>(tile_index);
  auto size = data.chunk_size();
  return {size * data.chunk_x() + index % size, size * data.chunk_y() + index / size};
}

}  // anonymous

World::World(worker::Connection& connection, worker::Dispatcher& dispatcher)
: connection_{connection}, dispatcher_{dispatcher}, player_id_{-1} {
  dispatcher_.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection_.SendInterestedComponents<schema::Position, schema::Chunk>(op.EntityId);
  });

  dispatcher_.OnAddComponent<schema::Position>(
      [&](const worker::AddComponentOp<schema::Position>& op) {
        entity_positions_.insert(std::make_pair(op.EntityId, coords(op.Data.coords())));
      });

  dispatcher_.OnRemoveComponent<schema::Position>(
      [&](const worker::RemoveComponentOp& op) { entity_positions_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::Position>(
      [&](const worker::ComponentUpdateOp<schema::Position>& op) {
        if (op.Update.coords()) {
          entity_positions_.insert(std::make_pair(op.EntityId, coords(*op.Update.coords())));
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
    direction += glm::vec3{-1.f, 0.f, 0.f};
  }
  if (input.held(Button::kRight)) {
    direction += glm::vec3{1.f, 0.f, 0.f};
  }
  if (input.held(Button::kDown)) {
    direction += glm::vec3{0.f, 0.f, -1.f};
  }
  if (input.held(Button::kUp)) {
    direction += glm::vec3{0.f, 0.f, 1.f};
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
  world_renderer_.render(renderer, glm::round(it->second), tile_map_);
}

void World::update_chunk(const schema::ChunkData& data) {
  for (std::size_t i = 0; i < data.tiles().size(); ++i) {
    tile_map_.insert(std::make_pair(tile_coords(data, i), data.tiles()[i]));
  }
}

void World::clear_chunk(const schema::ChunkData& data) {
  for (std::size_t i = 0; i < data.tiles().size(); ++i) {
    tile_map_.erase(tile_coords(data, i));
  }
}

}  // ::world
}  // ::gloam
