#include "common/src/core/tile_map.h"
#include "common/src/common/conversions.h"
#include <improbable/worker.h>

namespace gloam {
namespace core {

void TileMap::register_callbacks(worker::Dispatcher& dispatcher) {
  dispatcher.OnAddComponent<schema::Chunk>([&](const worker::AddComponentOp<schema::Chunk>& op) {
    chunk_map_.emplace(op.EntityId, op.Data);
    update_chunk(op.Data);
  });

  dispatcher.OnRemoveComponent<schema::Chunk>([&](const worker::RemoveComponentOp& op) {
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
