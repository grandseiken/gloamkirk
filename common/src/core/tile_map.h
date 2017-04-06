#ifndef GLOAM_COMMON_SRC_CORE_TILE_MAP_H
#define GLOAM_COMMON_SRC_CORE_TILE_MAP_H
#include "common/src/common/hashes.h"
#include <glm/vec2.hpp>
#include <schema/chunk.h>
#include <unordered_map>

namespace worker {
class Dispatcher;
}  // ::worker

namespace gloam {
namespace core {

class TileMap {
public:
  // Update the collision map based on callbacks from the dispatcher.
  void register_callbacks(worker::Dispatcher& dispatcher);

  bool has_changed() const;
  const std::unordered_map<glm::ivec2, schema::Tile>& get() const;

private:
  void update_chunk(const schema::ChunkData& data);
  void clear_chunk(const schema::ChunkData& data);

  std::unordered_map<worker::EntityId, schema::ChunkData> chunk_map_;
  std::unordered_map<glm::ivec2, schema::Tile> tile_map_;
  mutable bool tile_map_changed_ = false;
};

}  // ::core
}  // ::gloam

#endif
