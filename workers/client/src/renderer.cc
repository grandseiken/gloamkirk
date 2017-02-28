#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/fog.h"
#include "workers/client/src/shaders/postprocess.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>

namespace gloam {
namespace {

std::vector<GLfloat> quad_vertices = {
    -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1,
};

std::vector<GLushort> quad_indices = {
    0, 2, 1, 1, 2, 3,
};

}  // anonymous

Renderer::Renderer()
: frame_{0}
, max_texture_size_{0}
, quad_data_{quad_vertices, quad_indices, GL_STATIC_DRAW}
, fog_program_{"fog",
               {glo::Shader{"fog_vertex", GL_VERTEX_SHADER, shaders::fog_vertex},
                glo::Shader{"fog_fragment", GL_FRAGMENT_SHADER, shaders::fog_fragment}}} {
  quad_data_.enable_attribute(0, 4, 0, 0);

  float simplex_gradient_lut[3 * shaders::simplex3_gradient_texture_size] = {};
  // 7x7 points over a square mapped onto an octahedron.
  std::size_t i = 0;
  for (int yi = 0; yi < 7; ++yi) {
    for (int xi = 0; xi < 7; ++xi) {
      // X and Y evenly distributed over [-1, 1].
      double x = (2 * xi - 6) / 7.;
      double y = (2 * yi - 6) / 7.;

      // H in [-1, 1] with H + |X| + |Y| = 1.
      double h = 1 - fabs(x) - fabs(y);

      // Swaps the lower corners of the pyramid to form the octahedron.
      double ax = x - (2 * floor(x) + 1) * (h <= 0 ? 1 : 0);
      double ay = y - (2 * floor(y) + 1) * (h <= 0 ? 1 : 0);

      // Normalize.
      double len = sqrt(ax * ax + ay * ay + h * h);
      ax = .5 + ax / (len * 2);
      ay = .5 + ay / (len * 2);
      h = .5 + h / (len * 2);

      simplex_gradient_lut[i++] = static_cast<float>(ax);
      simplex_gradient_lut[i++] = static_cast<float>(ay);
      simplex_gradient_lut[i++] = static_cast<float>(h);
    }
  }

  // We use a LUT for simplex randomization where possible, since we can generate better random
  // numbers than in GLSL.
  float simplex_permutation_lut[shaders::simplex3_lut_permutation_texture_size] = {};
  const std::uint64_t lut_prime_factor = shaders::simplex3_lut_permutation_prime_factor;
  const std::uint64_t lut_ring_size = lut_prime_factor * lut_prime_factor;

  for (i = 0; i < shaders::simplex3_lut_permutation_texture_size; ++i) {
    auto x = (static_cast<std::uint64_t>(i) % lut_ring_size);
    auto out = (1 + x * (1 + 2 * x * lut_prime_factor)) % lut_ring_size;
    double f = static_cast<double>(out) / shaders::simplex3_lut_permutation_texture_size;
    simplex_permutation_lut[i] = static_cast<float>(f);
  }

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  simplex_gradient_lut_.create_1d(shaders::simplex3_gradient_texture_size, 3, simplex_gradient_lut);
  simplex_permutation_lut_.create_1d(shaders::simplex3_lut_permutation_texture_size, 1,
                                     simplex_permutation_lut);
}

void Renderer::resize(const glm::ivec2& dimensions) {
  static const int target_width = 720;
  auto target_scale = static_cast<int>(round(dimensions.x / static_cast<float>(target_width)));

  dimensions_ = dimensions;
  framebuffer_dimensions_ =
      dimensions / glm::ivec2{target_scale} + dimensions % glm::ivec2{target_scale};
}

void Renderer::render_frame() const {
  glViewport(0, 0, dimensions_.x, dimensions_.y);
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  glm::vec2 dimensions = dimensions_;
  auto program = fog_program_.use();
  set_simplex3_uniforms(program);
  glUniform1f(program.uniform("frame"), static_cast<float>(frame_++));
  glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
  quad_data_.draw();
}

void Renderer::set_simplex3_uniforms(const glo::ActiveProgram& program) const {
  glUniform1i(program.uniform("simplex3_use_permutation_lut"),
              uint32_t(max_texture_size_) >= shaders::simplex3_lut_permutation_texture_size);

  program.uniform_texture("simplex3_gradient_lut", simplex_gradient_lut_);
  program.uniform_texture("simplex3_permutation_lut", simplex_permutation_lut_);
}

}  // ::gloam
