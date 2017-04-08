#include "common/src/core/tile_map.h"
#include "common/src/common/conversions.h"
#include <improbable/worker.h>

#include <glm/glm.hpp>
#include <iostream>

namespace gloam {
namespace core {

void TileMap::register_callbacks(worker::Connection& connection, worker::Dispatcher& dispatcher) {
  dispatcher.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection.SendComponentInterest(op.EntityId, {{schema::Chunk::ComponentId, {true}}});
  });

  dispatcher.OnAddEntity([&](const worker::AddEntityOp& op) {
    std::cerr << "got add entity for " << op.EntityId << "\n";
  });

  dispatcher.OnRemoveEntity([&](const worker::RemoveEntityOp& op) {
    std::cerr << "got remove entity for " << op.EntityId << "\n";
  });

  dispatcher.OnAddComponent<schema::Chunk>([&](const worker::AddComponentOp<schema::Chunk>& op) {
    std::cerr << "got add chunk for " << op.EntityId << "\n";
    chunk_map_.emplace(op.EntityId, op.Data);
    update_chunk(op.Data);
  });

  dispatcher.OnRemoveComponent<schema::Chunk>([&](const worker::RemoveComponentOp& op) {
    std::cerr << "got remove chunk for " << op.EntityId << "\n";
    auto it = chunk_map_.find(op.EntityId);
    if (it != chunk_map_.end()) {
      clear_chunk(it->second);
      chunk_map_.erase(op.EntityId);
    }
  });

  dispatcher.OnComponentUpdate<schema::Chunk>(
      [&](const worker::ComponentUpdateOp<schema::Chunk>& op) {
        auto it = chunk_map_.find(op.EntityId);
        if (it != chunk_map_.end()) {
          op.Update.ApplyTo(it->second);
          update_chunk(it->second);
        }
      });
}

bool TileMap::has_changed() const {
  bool result = tile_map_changed_;
  if (result) {
    glm::ivec2 min;
    glm::ivec2 max;
    bool first = true;
    for (const auto& pair : chunk_map_) {
      glm::ivec2 v = {pair.second.chunk_x(), pair.second.chunk_y()};
      if (first) {
        first = false;
        min = v;
        max = v;
      }
      min = glm::min(min, v);
      max = glm::max(max, v);
    }
    std::cerr << "checked out from " << min.x << ", " << min.y << " -> " << max.x << ", " << max.y
              << ":\n";
    for (auto y = 5; y >= -5; --y) {
      for (auto x = -5; x <= 5; ++x) {
        bool exist = false;
        worker::EntityId id = 0;
        for (const auto& pair : chunk_map_) {
          if (glm::ivec2{pair.second.chunk_x(), pair.second.chunk_y()} == glm::ivec2{x, y}) {
            id = pair.first;
            exist = true;
            break;
          }
        }
        std::cerr << (exist ? (id < 10 ? " 0" : " ") + std::to_string(id) : "   ");
      }
      std::cerr << "\n";
    }
    std::cerr << "\n";
  }
  tile_map_changed_ = false;
  return result;
}

const std::unordered_map<glm::ivec2, schema::Tile>& TileMap::get() const {
  return tile_map_;
}

void TileMap::update_chunk(const schema::ChunkData& data) {
  for (std::size_t i = 0; i < data.tiles().size(); ++i) {
    auto key = common::tile_coords(data, i);
    auto it = tile_map_.find(key);
    if (it == tile_map_.end()) {
      tile_map_.emplace(key, data.tiles()[i]);
    } else {
      tile_map_.emplace_hint(tile_map_.erase(it), key, data.tiles()[i]);
    }
  }
  tile_map_changed_ = true;
}

void TileMap::clear_chunk(const schema::ChunkData& data) {
  for (std::size_t i = 0; i < data.tiles().size(); ++i) {
    tile_map_.erase(common::tile_coords(data, i));
  }
  tile_map_changed_ = true;
}

}  // ::core
}  // ::gloam
