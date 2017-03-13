#ifndef GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_RENDERER_H
#define GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_RENDERER_H
#include "common/src/common/hashes.h"
#include "workers/client/src/glo.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>
#include <vector>

namespace gloam {
class Renderer;
namespace schema {
class Tile;
}  // ::schema

namespace world {
struct Light {
  glm::vec3 world;
  float intensity;
};

class WorldRenderer {
public:
  WorldRenderer();
  void render(const Renderer& renderer, const glm::vec3& camera, const std::vector<Light>& lights,
              const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const;

private:
  void create_framebuffers(const glm::ivec2& aa_dimensions,
                           const glm::ivec2& protrusion_dimensions) const;

  glo::Program protrusion_program_;
  glo::Program world_program_;
  glo::Program material_program_;
  glo::Program light_program_;
  glo::Program fog_program_;
  mutable std::unique_ptr<glo::Framebuffer> protrusion_buffer_;
  mutable std::unique_ptr<glo::Framebuffer> world_buffer_;
  mutable std::unique_ptr<glo::Framebuffer> material_buffer_;
  mutable std::unique_ptr<glo::Framebuffer> composition_buffer_;
};

}  // ::world
}  // ::gloam

#endif
