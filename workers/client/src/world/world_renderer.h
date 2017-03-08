#ifndef GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_RENDERER_H
#define GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_RENDERER_H
#include "common/src/common/hashes.h"
#include "workers/client/src/glo.h"
#include <glm/vec3.hpp>

namespace gloam {
class Renderer;
namespace schema {
class Tile;
}  // ::schema

namespace world {

class WorldRenderer {
public:
  WorldRenderer();
  void render(const Renderer& renderer, const glm::vec3& camera,
              const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const;

private:
  glo::Program world_program_;
};

}  // ::world
}  // ::gloam

#endif