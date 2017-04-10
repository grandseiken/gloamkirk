#ifndef GLOAM_WORKERS_CLIENT_SRC_RENDER_H
#define GLOAM_WORKERS_CLIENT_SRC_RENDER_H
#include "workers/client/src/glo.h"
#include <SFML/Graphics.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <cstdint>
#include <memory>

namespace gloam {
namespace {
const glm::ivec2 max_resolution = {720, 480};
const glm::ivec2 native_resolution = {640, 360};
}  // anonymous

struct TextureImage {
  glo::Texture texture;
  glm::ivec2 dimensions;
};

TextureImage load_texture(const std::string& path);

class Renderer {
public:
  Renderer();

  // Rendering flow.
  glm::ivec2 framebuffer_dimensions() const;
  std::uint64_t frame() const;

  void resize(const glm::ivec2& dimensions);
  void begin_frame() const;
  void end_frame() const;

  // Utility functions for common rendering tasks.
  void set_default_render_states() const;
  void set_dither_translation(const glm::ivec2& translation) const;
  void set_simplex3_uniforms(const glo::ActiveProgram& program) const;

  void draw_quad() const;
  void draw_quad_colour(const glm::vec4& colour) const;

  std::uint32_t text_width(const std::string& text) const;
  void draw_text(const std::string& text, const glm::ivec2& position,
                 const glm::vec4& colour) const;

private:
  mutable glm::ivec2 dither_translation_;
  mutable std::unique_ptr<glo::ActiveFramebuffer> draw_;

  int target_upscale_;
  glm::ivec2 viewport_dimensions_;
  std::unique_ptr<glo::Framebuffer> framebuffer_;
  std::unique_ptr<glo::Framebuffer> postbuffer_;

  GLint max_texture_size_;
  glo::VertexData quad_data_;
  glo::Program text_program_;
  glo::Program post_program_;
  glo::Program quad_colour_program_;
  glo::Program quad_texture_program_;
  glo::Texture simplex_gradient_lut_;
  glo::Texture simplex_permutation_lut_;
  glo::Texture a_dither_matrix_;
  TextureImage text_image_;
};

}  // ::gloam

#endif
