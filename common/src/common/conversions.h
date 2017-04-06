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

inline improbable::math::Coordinates coords(const glm::vec3& coords) {
  return {coords.x, coords.y, coords.z};
}

inline glm::ivec2 tile_coords(const schema::ChunkData& data, std::size_t tile_index) {
  auto index = static_cast<std::int32_t>(tile_index);
  auto size = data.chunk_size();
  return {size * data.chunk_x() + index % size, size * data.chunk_y() + index / size};
}

}  // ::common
}  // ::gloam

#endif
