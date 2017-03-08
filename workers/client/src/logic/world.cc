#include "workers/client/src/logic/world.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/world.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <schema/common.h>
#include <algorithm>

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

  static float a = 0;
  a += 1 / 128.f;

  auto dimensions = renderer.framebuffer_dimensions();
  // Not sure what exact values we need for z-planes to be correct, this should do for now.
  auto camera_distance =
      std::max(static_cast<float>(kTileSize), 2 * renderer.framebuffer_dimensions().y);

  auto ortho = glm::ortho(dimensions.x / 2, -dimensions.x / 2, -dimensions.y / 2, dimensions.y / 2,
                          1 / camera_distance, 2 * camera_distance);
  auto look_at = glm::lookAt(position + camera_distance * glm::vec3{cos(a), 1.f, sin(a)}, position,
                             glm::vec3{0.f, 1.f, 0.f});
  auto camera_matrix = ortho * look_at;

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
    glm::vec4 top_normal = {0, 1., 0., 1.};

    add_vec4({min.x, height, min.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({min.x, height, max.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({max.x, height, min.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({max.x, height, max.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_quad();

    auto jt = tile_map_.find(coord - glm::ivec2{0, 1});
    if (jt != tile_map_.end() && jt->second.height() != pair.second.height()) {
      auto y = coord.y * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.x * kTileSize, next_height};
      max = glm::vec2{(1 + coord.x) * kTileSize, height};
      glm::vec4 side_normal = {0., 0., jt->second.height() > pair.second.height() ? 1. : -1., 1.};

      add_vec4({min.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({min.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({max.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({max.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_quad();
    }

    jt = tile_map_.find(coord - glm::ivec2{1, 0});
    if (jt != tile_map_.end()) {
      auto x = coord.x * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.y * kTileSize, next_height};
      max = glm::vec2{(1 + coord.y) * kTileSize, height};
      glm::vec4 side_normal = {jt->second.height() > pair.second.height() ? 1. : -1., 0., 0., 1.};

      add_vec4({x, min.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, min.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, max.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, max.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_quad();
    }
  }

  glm::vec4 light_position = {position + glm::vec3{0.f, 64.f, 0.f}, 1.f};
  glo::VertexData vertex_data{data, indices, GL_DYNAMIC_DRAW};
  vertex_data.enable_attribute(0, 4, 9, 0);
  vertex_data.enable_attribute(1, 4, 9, 4);
  vertex_data.enable_attribute(2, 1, 9, 8);

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  auto program = world_program_.use();
  glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
  glUniform4fv(program.uniform("light_world"), 1, glm::value_ptr(light_position));
  glUniform1f(program.uniform("light_intensity"), 8.f);
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
