#include "workers/client/src/logic/world.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/world.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <schema/common.h>

namespace gloam {
namespace logic {
namespace {
// Tile size in pixels.
const std::int32_t kTileSize = 32;

glm::ivec2 tile_coords(const schema::ChunkData& data, std::size_t tile_index) {
  auto index = static_cast<std::int32_t>(tile_index);
  auto size = data.chunk_size();
  return {size * data.chunk_x() + index % size, size * data.chunk_y() + index / size};
}

}  // anonymous

World::World(worker::Connection& connection, worker::Dispatcher& dispatcher)
: connection_{connection}
, dispatcher_{dispatcher}
, player_id_{-1}
, world_program_{"world",
                 {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                 {"world_fragment", GL_FRAGMENT_SHADER, shaders::world_fragment}} {
  dispatcher_.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection_.SendInterestedComponents<schema::Position, schema::Chunk>(op.EntityId);
  });

  dispatcher_.OnAddComponent<schema::Position>(
      [&](const worker::AddComponentOp<schema::Position>& op) {
        entity_positions_.insert(std::make_pair(op.EntityId, op.Data.coords()));
      });

  dispatcher_.OnRemoveComponent<schema::Position>(
      [&](const worker::RemoveComponentOp& op) { entity_positions_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::Position>(
      [&](const worker::ComponentUpdateOp<schema::Position>& op) {
        if (op.Update.coords()) {
          entity_positions_.insert(std::make_pair(op.EntityId, *op.Update.coords()));
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

void World::render(const Renderer& renderer) const {
  auto it = entity_positions_.find(player_id_);
  if (it == entity_positions_.end()) {
    return;
  }

  auto position = glm::vec3{static_cast<float>(it->second.x()), static_cast<float>(it->second.y()),
                            static_cast<float>(it->second.z())};
  auto dimensions = renderer.framebuffer_dimensions();
  auto ortho = glm::ortho(-dimensions.x / 2, dimensions.x / 2, -dimensions.y / 2, dimensions.y / 2,
                          1.f / 1024, 256.f);
  auto look_at =
      glm::lookAt(position + 8.f * glm::vec3{0.f, 1.f, -2.f}, position, glm::vec3{0.f, 1.f, 0.f});
  auto camera_matrix = ortho * look_at;

  static const glm::vec4 top_colour = {.0625, .5, .25, 1.};
  static const glm::vec4 side_colour = {.375, .375, .5, 1.};
  std::vector<float> data;
  std::vector<GLushort> indices;
  GLushort index = 0;

  auto add_vec4 = [&](const glm::vec4& v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
    data.push_back(v.w);
  };

  auto add_quad = [&] {
    indices.push_back(index + 0);
    indices.push_back(index + 2);
    indices.push_back(index + 1);
    indices.push_back(index + 1);
    indices.push_back(index + 2);
    indices.push_back(index + 3);
    index += 4;
  };

  for (const auto& pair : tile_map_) {
    const auto& coord = pair.first;
    auto min = glm::vec2{coord * kTileSize};
    auto max = glm::vec2{(coord + glm::ivec2{1, 1}) * kTileSize};
    auto height = static_cast<float>(pair.second.height() * kTileSize);

    add_vec4({min.x, height, min.y, 1.f});
    add_vec4(top_colour);
    add_vec4({min.x, height, max.y, 1.f});
    add_vec4(top_colour);
    add_vec4({max.x, height, min.y, 1.f});
    add_vec4(top_colour);
    add_vec4({max.x, height, max.y, 1.f});
    add_vec4(top_colour);
    add_quad();

    auto it = tile_map_.find(coord - glm::ivec2{0, 1});
    if (it == tile_map_.end()) {
      continue;
    }
    auto y = coord.y * kTileSize;
    auto next_height = static_cast<float>(it->second.height() * kTileSize);
    min = glm::vec2{coord.x * kTileSize, next_height};
    max = glm::vec2{(1 + coord.x) * kTileSize, height};

    add_vec4({min.x, min.y, y, 1.f});
    add_vec4(side_colour);
    add_vec4({min.x, max.y, y, 1.f});
    add_vec4(side_colour);
    add_vec4({max.x, min.y, y, 1.f});
    add_vec4(side_colour);
    add_vec4({max.x, max.y, y, 1.f});
    add_vec4(side_colour);
    add_quad();
  }

  glo::VertexData vertex_data{data, indices, GL_DYNAMIC_DRAW};
  vertex_data.enable_attribute(0, 4, 8, 0);
  vertex_data.enable_attribute(1, 4, 8, 4);

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  auto program = world_program_.use();
  glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
  renderer.set_simplex3_uniforms(program);
  vertex_data.draw();
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

}  // ::logic
}  // ::gloam
