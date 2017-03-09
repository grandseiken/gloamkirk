#include "workers/client/src/world/world_renderer.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/light.h"
#include "workers/client/src/shaders/world.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <schema/chunk.h>
#include <algorithm>

namespace gloam {
namespace world {
namespace {
// Tile size in pixels.
const std::int32_t kTileSize = 32;

glo::VertexData generate_vertex_data(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) {
  std::vector<float> data;
  std::vector<GLushort> indices;
  GLushort index = 0;

  auto add_vec4 = [&](const glm::vec4& v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
    data.push_back(v.w);
  };

  auto add_quad = [&] {
    indices.push_back(index + 0);
    indices.push_back(index + 2);
    indices.push_back(index + 1);
    indices.push_back(index + 1);
    indices.push_back(index + 2);
    indices.push_back(index + 3);
    index += 4;
  };

  for (const auto& pair : tile_map) {
    const auto& coord = pair.first;
    auto min = glm::vec2{coord * kTileSize};
    auto max = glm::vec2{(coord + glm::ivec2{1, 1}) * kTileSize};
    auto height = static_cast<float>(pair.second.height() * kTileSize);
    glm::vec4 top_normal = {0, 1., 0., 1.};

    add_vec4({min.x, height, min.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({min.x, height, max.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({max.x, height, min.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({max.x, height, max.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_quad();

    auto jt = tile_map.find(coord - glm::ivec2{0, 1});
    if (jt != tile_map.end() && jt->second.height() != pair.second.height()) {
      auto y = coord.y * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.x * kTileSize, next_height};
      max = glm::vec2{(1 + coord.x) * kTileSize, height};
      glm::vec4 side_normal = {0., 0., jt->second.height() > pair.second.height() ? 1. : -1., 1.};

      add_vec4({min.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({min.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({max.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({max.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_quad();
    }

    jt = tile_map.find(coord - glm::ivec2{1, 0});
    if (jt != tile_map.end()) {
      auto x = coord.x * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.y * kTileSize, next_height};
      max = glm::vec2{(1 + coord.y) * kTileSize, height};
      glm::vec4 side_normal = {jt->second.height() > pair.second.height() ? 1. : -1., 0., 0., 1.};

      add_vec4({x, min.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, min.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, max.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, max.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_quad();
    }
  }

  glo::VertexData result{data, indices, GL_DYNAMIC_DRAW};
  result.enable_attribute(0, 4, 9, 0);
  result.enable_attribute(1, 4, 9, 4);
  result.enable_attribute(2, 1, 9, 8);
  return result;
}

}  // anonymous

WorldRenderer::WorldRenderer()
: world_program_{"world",
                 {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                 {"world_fragment", GL_FRAGMENT_SHADER, shaders::world_fragment}}
, light_program_{"light",
                 {"light_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                 {"light_fragment", GL_FRAGMENT_SHADER, shaders::light_fragment}} {}

void WorldRenderer::render(const Renderer& renderer, const glm::vec3& camera,
                           const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const {
  if (!gbuffer_ || gbuffer_->dimensions() != renderer.framebuffer_dimensions()) {
    gbuffer_.reset(new glo::Framebuffer{renderer.framebuffer_dimensions()});
    // World position buffer.
    gbuffer_->add_colour_buffer(/* high-precision RGB */ true);
    // Normal buffer.
    gbuffer_->add_colour_buffer(/* high-precision RGB */ true);
    // Colour buffer.
    gbuffer_->add_colour_buffer(/* RGBA */ false);
    // Depth buffer.
    gbuffer_->add_depth_stencil_buffer();
    gbuffer_->check_complete();
  }

  // Not sure what exact values we need for z-planes to be correct, this should do for now.
  auto camera_distance =
      static_cast<float>(std::max(kTileSize, 2 * renderer.framebuffer_dimensions().y));
  glm::vec2 dimensions = renderer.framebuffer_dimensions();

  auto ortho = glm::ortho(dimensions.x / 2, -dimensions.x / 2, -dimensions.y / 2, dimensions.y / 2,
                          1 / camera_distance, 2 * camera_distance);
  auto look_at = glm::lookAt(camera + camera_distance * glm::vec3{.5f, 1.f, -1.f}, camera,
                             glm::vec3{0.f, 1.f, 0.f});
  auto camera_matrix = ortho * look_at;

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  {
    auto draw = gbuffer_->draw();
    glViewport(0, 0, renderer.framebuffer_dimensions().x, renderer.framebuffer_dimensions().y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = world_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
    renderer.set_simplex3_uniforms(program);
    generate_vertex_data(tile_map).draw();
  }

  static float a = 0;
  a += 1 / 128.f;
  auto light_position = camera + glm::vec3{256.f * cos(a), 32.f, 256.f * sin(a)};
  renderer.set_default_render_states();

  // Should be converted to draw the lights as individuals quads in a single draw call.
  auto program = light_program_.use();
  program.uniform_texture("gbuffer_world", gbuffer_->colour_textures()[0]);
  program.uniform_texture("gbuffer_normal", gbuffer_->colour_textures()[1]);
  program.uniform_texture("gbuffer_colour", gbuffer_->colour_textures()[2]);
  glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
  glUniform3fv(program.uniform("light_world"), 1, glm::value_ptr(light_position));
  glUniform1f(program.uniform("light_intensity"), 1.f);
  renderer.draw_quad();
}

}  // ::world
}  // ::gloam