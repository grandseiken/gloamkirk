#ifndef GLOAM_COMMON_SRC_COMMON_CONVERSIONS_H
#define GLOAM_COMMON_SRC_COMMON_CONVERSIONS_H
#include "common/src/common/hashes.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <improbable/standard_library.h>
#include <schema/chunk.h>
#include <unordered_map>

namespace gloam {
namespace common {

inline glm::vec3 coords(const improbable::math::Coordinates& schema_coords) {
  return {schema_coords.x(), schema_coords.y(), schema_coords.z()};
}

inline glm::ivec2 tile_coords(const schema::ChunkData& data, std::size_t tile_index) {
  auto index = static_cast<std::int32_t>(tile_index);
  auto size = data.chunk_size();
  return {size * data.chunk_x() + index % size, size * data.chunk_y() + index / size};
}

inline void update_chunk(std::unordered_map<glm::ivec2, schema::Tile>& tile_map,
                         const schema::ChunkData& chunk_data) {
  for (std::size_t i = 0; i < chunk_data.tiles().size(); ++i) {
    auto key = tile_coords(chunk_data, i);
    auto it = tile_map.find(key);
    if (it == tile_map.end()) {
      tile_map.emplace(key, chunk_data.tiles()[i]);
    } else {
      tile_map.emplace_hint(tile_map.erase(it), key, chunk_data.tiles()[i]);
    }
  }
}

inline void clear_chunk(std::unordered_map<glm::ivec2, schema::Tile>& tile_map,
                        const schema::ChunkData& chunk_data) {
  for (std::size_t i = 0; i < chunk_data.tiles().size(); ++i) {
    tile_map.erase(tile_coords(chunk_data, i));
  }
}

}  // ::common
}  // ::gloam

#endif
