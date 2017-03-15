#ifndef GLOAM_COMMON_SRC_COLLISION_COLLISION_H
#define GLOAM_COMMON_SRC_COLLISION_COLLISION_H
#include "common/src/common/hashes.h"
#include <glm/vec2.hpp>
#include <schema/chunk.h>
#include <unordered_map>

namespace gloam {
namespace collision {

class Collision {
public:
  void update(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map);
};

}  // ::collision
}  // ::gloam

#endif
