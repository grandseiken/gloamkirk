#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/fog.h"
#include "workers/client/src/shaders/post.h"
#include "workers/client/src/shaders/upscale.h"
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

enum class a_dither_pattern {
  ADD,
  ADD_RGBSPLIT,
};

float a_dither_compute(a_dither_pattern pattern, std::int32_t x, std::int32_t y, std::int32_t c) {
  switch (pattern) {
  case a_dither_pattern::ADD:
    return ((x + y * 237) * 119 & 255) / float(shaders::a_dither_res);
  case a_dither_pattern::ADD_RGBSPLIT:
    return (((x + c * 67) + y * 236) * 119 & 255) / float(shaders::a_dither_res);
  default:
    return .5f;
  }
}

}  // anonymous

Renderer::Renderer()
: frame_{0}
, target_upscale_{1}
, max_texture_size_{0}
, quad_data_{quad_vertices, quad_indices, GL_STATIC_DRAW}
, fog_program_{"fog",
               {glo::Shader{"fog_vertex", GL_VERTEX_SHADER, shaders::fog_vertex},
                glo::Shader{"fog_fragment", GL_FRAGMENT_SHADER, shaders::fog_fragment}}}
, post_program_{"post",
                {glo::Shader{"post_vertex", GL_VERTEX_SHADER, shaders::post_vertex},
                 glo::Shader{"post_fragment", GL_FRAGMENT_SHADER, shaders::post_fragment}}}
, upscale_program_{
      "upscale",
      {glo::Shader{"upscale_vertex", GL_VERTEX_SHADER, shaders::upscale_vertex},
       glo::Shader{"upscale_fragment", GL_FRAGMENT_SHADER, shaders::upscale_fragment}}} {
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

  constexpr auto pattern = a_dither_pattern::ADD_RGBSPLIT;
  float a_dither_matrix[3 * shaders::a_dither_res * shaders::a_dither_res] = {};
  std::size_t n = 0;
  for (std::int32_t x = 0; x < shaders::a_dither_res; ++x) {
    for (std::int32_t y = 0; y < shaders::a_dither_res; ++y) {
      a_dither_matrix[n++] = a_dither_compute(pattern, x, y, 0);
      a_dither_matrix[n++] = a_dither_compute(pattern, x, y, 1);
      a_dither_matrix[n++] = a_dither_compute(pattern, x, y, 2);
    }
  }
  a_dither_matrix_.create_2d({shaders::a_dither_res, shaders::a_dither_res}, 3, a_dither_matrix);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  simplex_gradient_lut_.create_1d(shaders::simplex3_gradient_texture_size, 3, simplex_gradient_lut);
  simplex_permutation_lut_.create_1d(shaders::simplex3_lut_permutation_texture_size, 1,
                                     simplex_permutation_lut);
}

void Renderer::resize(const glm::ivec2& dimensions) {
  if (dimensions == viewport_dimensions_) {
    return;
  }
  viewport_dimensions_ = dimensions;

  static const int target_width = 720;
  target_upscale_ = static_cast<int>(round(dimensions.x / static_cast<float>(target_width)));
  framebuffer_dimensions_ = dimensions / glm::ivec2{target_upscale_};

  framebuffer_.reset(new glo::Framebuffer{framebuffer_dimensions_, true, false});
  postbuffer_.reset(new glo::Framebuffer{framebuffer_dimensions_, false, false});
}

void Renderer::render_frame() const {
  ++frame_;

  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glClearColor(0, 0, 0, 0);

  {
    auto draw = framebuffer_->draw();
    glViewport(0, 0, framebuffer_dimensions_.x, framebuffer_dimensions_.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    auto program = fog_program_.use();
    set_simplex3_uniforms(program);
    glUniform1f(program.uniform("frame"), static_cast<float>(frame_));
    quad_data_.draw();
  }

  {
    auto draw = postbuffer_->draw();
    glViewport(0, 0, framebuffer_dimensions_.x, framebuffer_dimensions_.y);

    auto program = post_program_.use();
    glUniform1f(program.uniform("frame"), static_cast<float>(frame_));
    program.uniform_texture("dither_matrix", a_dither_matrix_);
    program.uniform_texture("source_framebuffer", framebuffer_->texture());
    quad_data_.draw();
  }

  glViewport(0, 0, viewport_dimensions_.x, viewport_dimensions_.y);
  glClear(GL_COLOR_BUFFER_BIT);

  auto dimensions = target_upscale_ * framebuffer_dimensions_;
  auto border = (viewport_dimensions_ - dimensions) / 2;
  glViewport(border.x, border.y, dimensions.x, dimensions.y);

  auto program = upscale_program_.use();
  program.uniform_texture("source_framebuffer", postbuffer_->texture());
  quad_data_.draw();
}

void Renderer::set_simplex3_uniforms(const glo::ActiveProgram& program) const {
  glUniform1i(program.uniform("simplex3_use_permutation_lut"),
              uint32_t(max_texture_size_) >= shaders::simplex3_lut_permutation_texture_size);

  program.uniform_texture("simplex3_gradient_lut", simplex_gradient_lut_);
  program.uniform_texture("simplex3_permutation_lut", simplex_permutation_lut_);
}

}  // ::gloam
