#include "common/src/collision/collision.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace gloam {
namespace collision {

void Collision::update(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) {
  glm::ivec3 min;
  glm::ivec3 max;

  bool first = true;
  for (const auto& pair : tile_map) {
    glm::ivec3 coords = {pair.first.x, pair.second.height(), pair.first.y};
    if (first) {
      min = max = coords;
    }
    min = glm::min(min, coords);
    max = glm::max(max, coords);
  }
}

}  // ::collision
}  // ::gloam
