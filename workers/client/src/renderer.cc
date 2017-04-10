#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/post.h"
#include "workers/client/src/shaders/simplex.h"
#include "workers/client/src/shaders/text.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

namespace gloam {
namespace {

std::vector<float> quad_vertices = {
    -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, 1, 1, 1, 1,
};

std::vector<GLuint> quad_indices = {
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

TextureImage load_texture(const std::string& path) {
  sf::Image image;
  image.loadFromFile(path);

  TextureImage result;
  result.dimensions = {image.getSize().x, image.getSize().y};
  result.texture.create_2d(result.dimensions, 4, GL_UNSIGNED_BYTE, image.getPixelsPtr());
  return result;
}

Renderer::Renderer()
: target_upscale_{1}
, max_texture_size_{0}
, quad_data_{quad_vertices, quad_indices, GL_STATIC_DRAW}
, text_program_{"text",
                {"text_vertex", GL_VERTEX_SHADER, shaders::text_vertex},
                {"text_fragment", GL_FRAGMENT_SHADER, shaders::text_fragment}}
, post_program_{"post",
                {"post_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                {"post_fragment", GL_FRAGMENT_SHADER, shaders::post_fragment}}
, quad_colour_program_{"quad_colour",
                       {"quad_verter", GL_VERTEX_SHADER, shaders::quad_vertex},
                       {"quad_colour_fragment", GL_FRAGMENT_SHADER, shaders::quad_colour_fragment}}
, quad_texture_program_{"quad_texture",
                        {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                        {"quad_texture_fragment", GL_FRAGMENT_SHADER,
                         shaders::quad_texture_fragment}}
, text_image_{load_texture("assets/text.png")} {
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
  a_dither_matrix_.create_2d({shaders::a_dither_res, shaders::a_dither_res}, 3, GL_FLOAT,
                             a_dither_matrix);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size_);
  simplex_gradient_lut_.create_1d(shaders::simplex3_gradient_texture_size, 3, GL_FLOAT,
                                  simplex_gradient_lut);
  simplex_permutation_lut_.create_1d(shaders::simplex3_lut_permutation_texture_size, 1, GL_FLOAT,
                                     simplex_permutation_lut);
}

glm::ivec2 Renderer::framebuffer_dimensions() const {
  return framebuffer_->dimensions();
}

void Renderer::resize(const glm::ivec2& dimensions) {
  if (dimensions == viewport_dimensions_) {
    return;
  }
  viewport_dimensions_ = dimensions;

  target_upscale_ = 1;
  glm::ivec2 framebuffer_dimensions;
  while (target_upscale_ < 256) {
    framebuffer_dimensions = dimensions / glm::ivec2{target_upscale_};
    if (framebuffer_dimensions.x <= max_resolution.x &&
        framebuffer_dimensions.y <= max_resolution.y) {
      break;
    }
    ++target_upscale_;
  }

  framebuffer_.reset(new glo::Framebuffer{framebuffer_dimensions});
  framebuffer_->add_colour_buffer(false);
  framebuffer_->add_depth_stencil_buffer();
  framebuffer_->check_complete();
  postbuffer_.reset(new glo::Framebuffer{framebuffer_dimensions});
  postbuffer_->add_colour_buffer(false);
  postbuffer_->check_complete();
}

void Renderer::begin_frame() const {
  dither_translation_ = {};
  set_default_render_states();
  draw_.reset(new glo::ActiveFramebuffer{framebuffer_->draw()});
  glViewport(0, 0, framebuffer_dimensions().x, framebuffer_dimensions().y);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Renderer::end_frame() const {
  set_default_render_states();
  draw_.reset();
  {
    auto draw = postbuffer_->draw();
    glViewport(0, 0, framebuffer_dimensions().x, framebuffer_dimensions().y);

    auto program = post_program_.use();
    glUniform2fv(program.uniform("dimensions"), 1,
                 glm::value_ptr(glm::vec2{framebuffer_->dimensions()}));
    glUniform2fv(program.uniform("translation"), 1,
                 glm::value_ptr(glm::vec2{dither_translation_.x, -dither_translation_.y}));
    program.uniform_texture("dither_matrix", a_dither_matrix_);
    program.uniform_texture("source_framebuffer", framebuffer_->colour_textures()[0]);
    draw_quad();
  }

  glViewport(0, 0, viewport_dimensions_.x, viewport_dimensions_.y);
  glClear(GL_COLOR_BUFFER_BIT);

  auto upscale = target_upscale_ * framebuffer_dimensions();
  auto border = (viewport_dimensions_ - upscale) / 2;
  glViewport(border.x, border.y, upscale.x, upscale.y);
  glm::vec2 upscale_dimensions = upscale;

  auto program = quad_texture_program_.use();
  glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(upscale_dimensions));
  program.uniform_texture("source_texture", postbuffer_->colour_textures()[0]);
  draw_quad();
}

void Renderer::set_default_render_states() const {
  glClearColor(0, 0, 0, 0);
  glClearDepth(1);
  glClearStencil(0);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

void Renderer::set_dither_translation(const glm::ivec2& translation) const {
  dither_translation_ = translation;
}

void Renderer::set_simplex3_uniforms(const glo::ActiveProgram& program) const {
  glUniform1i(program.uniform("simplex3_use_permutation_lut"),
              uint32_t(max_texture_size_) >= shaders::simplex3_lut_permutation_texture_size);

  program.uniform_texture("simplex3_gradient_lut", simplex_gradient_lut_);
  program.uniform_texture("simplex3_permutation_lut", simplex_permutation_lut_);
}

void Renderer::draw_quad() const {
  quad_data_.draw();
}

void Renderer::draw_quad_colour(const glm::vec4& colour) const {
  set_default_render_states();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  auto program = quad_colour_program_.use();
  glUniform4fv(program.uniform("colour"), 1, glm::value_ptr(colour));
  draw_quad();
}

std::uint32_t Renderer::text_width(const std::string& text) const {
  std::uint32_t result = 0;
  bool first = true;
  for (std::uint8_t c : text) {
    result += shaders::text_widths[c];
    if (first) {
      first = false;
    } else {
      result += 1;
    }
  }
  return result;
}

void Renderer::draw_text(const std::string& text, const glm::ivec2& position,
                         const glm::vec4& colour) const {
  std::vector<float> data;
  std::vector<GLuint> indices;
  GLuint index = 0;

  std::uint32_t x = 0;
  for (std::uint8_t c : text) {
    auto char_width = shaders::text_widths[c];

    data.push_back(static_cast<float>(x + position.x));
    data.push_back(static_cast<float>(position.y + shaders::text_height));
    data.push_back(static_cast<float>((c % 32) * shaders::text_size));
    data.push_back(static_cast<float>((c / 32) * shaders::text_size + shaders::text_size));

    data.push_back(static_cast<float>(x + position.x));
    data.push_back(static_cast<float>(position.y));
    data.push_back(static_cast<float>((c % 32) * shaders::text_size));
    data.push_back(static_cast<float>((c / 32) * shaders::text_size + shaders::text_border));

    data.push_back(static_cast<float>(x + position.x + char_width));
    data.push_back(static_cast<float>(position.y + shaders::text_height));
    data.push_back(static_cast<float>((c % 32) * shaders::text_size + char_width));
    data.push_back(static_cast<float>((c / 32) * shaders::text_size + shaders::text_size));

    data.push_back(static_cast<float>(x + position.x + char_width));
    data.push_back(static_cast<float>(position.y));
    data.push_back(static_cast<float>((c % 32) * shaders::text_size + char_width));
    data.push_back(static_cast<float>((c / 32) * shaders::text_size + shaders::text_border));

    indices.push_back(index + 0);
    indices.push_back(index + 2);
    indices.push_back(index + 1);
    indices.push_back(index + 1);
    indices.push_back(index + 2);
    indices.push_back(index + 3);
    x += char_width + 1;
    index += 4;
  }

  set_default_render_states();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glo::VertexData vertex_data{data, indices, GL_DYNAMIC_DRAW};
  auto program = text_program_.use();
  program.uniform_texture("text_texture", text_image_.texture);
  glUniform2fv(program.uniform("text_dimensions"), 1,
               glm::value_ptr(glm::vec2{text_image_.dimensions}));
  glUniform2fv(program.uniform("framebuffer_dimensions"), 1,
               glm::value_ptr(glm::vec2{framebuffer_->dimensions()}));
  glUniform4fv(program.uniform("text_colour"), 1, glm::value_ptr(colour));
  vertex_data.enable_attribute(0, 2, 4, 0);
  vertex_data.enable_attribute(1, 2, 4, 2);
  vertex_data.draw();
}

}  // ::gloam
