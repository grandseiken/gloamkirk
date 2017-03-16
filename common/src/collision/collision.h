#ifndef GLOAM_COMMON_SRC_COLLISION_COLLISION_H
#define GLOAM_COMMON_SRC_COLLISION_COLLISION_H
#include "common/src/common/hashes.h"
#include <glm/vec2.hpp>
#include <schema/chunk.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gloam {
namespace collision {

class Collision {
public:
  void update(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map);

private:
  // 2D edges are in clockwise order (i.e. outward-facing normals on the left).
  struct Edge {
    glm::vec2 a;
    glm::vec2 b;
  };

  // Layers of world geometry.
  struct LayerData {
    std::vector<Edge> edges;
  };

  std::unordered_map<std::uint32_t, LayerData> layers_;
};

}  // ::collision
}  // ::gloam

#endif
