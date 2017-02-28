#ifndef GLOAM_CLIENT_SRC_RENDER_H
#define GLOAM_CLIENT_SRC_RENDER_H
#include "workers/client/src/glo.h"
#include <glm/vec2.hpp>

namespace gloam {

class Renderer {
public:
  Renderer();

  void resize(const glm::ivec2& dimensions);
  void render_frame() const;

private:
  void set_simplex3_uniforms(const glo::ActiveProgram& program) const;

  mutable std::uint64_t frame_;
  glm::ivec2 framebuffer_dimensions_;
  glm::ivec2 dimensions_;
  GLint max_texture_size_;

  glo::VertexData quad_data_;
  glo::Program fog_program_;
  glo::Texture simplex_gradient_lut_;
  glo::Texture simplex_permutation_lut_;
};

}  // ::gloam

#endif
