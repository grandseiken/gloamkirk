#ifndef GLOAM_COMMON_SRC_CORE_COLLISION_H
#define GLOAM_COMMON_SRC_CORE_COLLISION_H
#include "common/src/common/hashes.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <schema/chunk.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gloam {
namespace core {
class TileMap;

// Diagonal-axis-aligned collision box, with origin at centre-bottom.
struct Box {
  // Half-width on each diagonal axis.
  float radius;
};

// 2D edges are in clockwise order (i.e. outward-facing normals on the left).
struct Edge {
  glm::vec2 a;
  glm::vec2 b;
};

class Collision {
public:
  // Recalculate terrain geometry from the tile map.
  void update(const TileMap& tile_map);

  // Project a collision box at position along the given XZ projection vector, and return the
  // modified, unimpeded projection.
  glm::vec2 project_xz(const Box& box, const glm::vec3& position,
                       const glm::vec2& projection) const;

private:
  // Layers of world geometry.
  struct LayerData {
    // All the edges in this layer.
    std::vector<Edge> edges;
    // Acceleration structure: map from tile coordinate to buckets of indexes into edge map, giving
    // all edges incident to the tile.
    std::unordered_map<glm::ivec2, std::vector<std::size_t>> tile_lookup;
  };

  std::unordered_map<std::uint32_t, LayerData> layers_;
  std::unordered_map<glm::ivec2, float> height_map_;
};

}  // ::core
}  // ::gloam

#endif
