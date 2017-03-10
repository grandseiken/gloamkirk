#include "workers/client/src/world/world_renderer.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/fog.h"
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
const std::int32_t kPixelLayers = 8;

glo::VertexData generate_world_data(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map,
                                    float pixel_height) {
  std::vector<float> data;
  std::vector<GLuint> indices;
  GLuint index = 0;

  auto add_vec4 = [&](const glm::vec4& v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
    data.push_back(v.w);
  };

  auto add_tri = [&](GLuint a, GLuint b, GLuint c) {
    indices.push_back(index + a);
    indices.push_back(index + b);
    indices.push_back(index + c);
  };

  auto add_quad = [&] {
    add_tri(0, 2, 1);
    add_tri(1, 2, 3);
    index += 4;
  };

  auto height_differs = [&](const glm::ivec2& coord, std::int32_t height) {
    auto it = tile_map.find(coord);
    return it != tile_map.end() && it->second.height() != height;
  };

  for (const auto& pair : tile_map) {
    const auto& coord = pair.first;
    auto min = glm::vec2{coord * kTileSize};
    auto max = glm::vec2{(coord + glm::ivec2{1, 1}) * kTileSize};
    auto height = static_cast<float>(pair.second.height() * kTileSize);
    glm::vec4 top_normal = {0, 1., 0., 1.};

    bool l_edge = height_differs(coord + glm::ivec2{-1, 0}, pair.second.height());
    bool r_edge = height_differs(coord + glm::ivec2{1, 0}, pair.second.height());
    bool b_edge = height_differs(coord + glm::ivec2{0, -1}, pair.second.height());
    bool t_edge = height_differs(coord + glm::ivec2{0, 1}, pair.second.height());
    bool tl_edge = height_differs(coord + glm::ivec2{-1, 1}, pair.second.height());
    bool tr_edge = height_differs(coord + glm::ivec2{1, 1}, pair.second.height());
    bool bl_edge = height_differs(coord + glm::ivec2{-1, -1}, pair.second.height());
    bool br_edge = height_differs(coord + glm::ivec2{1, -1}, pair.second.height());

    for (std::int32_t pixel_layer = 0; pixel_layer < kPixelLayers; ++pixel_layer) {
      auto world_height = pixel_layer * pixel_height;
      auto world_offset = glm::vec4{0.f, pixel_layer * pixel_height, 0.f, 0.f};

      add_vec4(world_offset + glm::vec4{min.x, height, min.y, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(l_edge || b_edge || bl_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{min.x, height, max.y, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(l_edge || t_edge || tl_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{max.x, height, min.y, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(r_edge || b_edge || br_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{max.x, height, max.y, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(r_edge || t_edge || tr_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{min.x, height, (min.y + max.y) / 2, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(l_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{(min.x + max.x) / 2, height, max.y, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(t_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{(min.x + max.x) / 2, height, min.y, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(b_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{max.x, height, (min.y + max.y) / 2, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(r_edge);
      data.push_back(world_height);

      add_vec4(world_offset + glm::vec4{(min.x + max.x) / 2, height, (min.y + max.y) / 2, 1.f});
      add_vec4(top_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(world_height);

      add_tri(0, 6, 4);
      add_tri(6, 8, 4);
      add_tri(4, 8, 1);
      add_tri(8, 5, 1);
      add_tri(8, 7, 5);
      add_tri(7, 3, 5);
      add_tri(6, 2, 8);
      add_tri(2, 7, 8);
      index += 9;
    }

    auto jt = tile_map.find(coord - glm::ivec2{0, 1});
    if (jt != tile_map.end() && jt->second.height() != pair.second.height()) {
      auto y = coord.y * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.x * kTileSize, next_height};
      max = glm::vec2{(1 + coord.x) * kTileSize, height};
      glm::vec4 side_normal = {0., 0., jt->second.height() > pair.second.height() ? 1. : -1., 1.};

      add_vec4({min.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_vec4({min.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_vec4({max.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_vec4({max.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
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
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_vec4({x, min.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_vec4({x, max.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_vec4({x, max.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(0);
      data.push_back(0);
      data.push_back(0);
      add_quad();
    }
  }

  glo::VertexData result{data, indices, GL_DYNAMIC_DRAW};
  result.enable_attribute(0, 4, 11, 0);
  result.enable_attribute(1, 4, 11, 4);
  result.enable_attribute(2, 3, 11, 8);
  return result;
}

glo::VertexData generate_fog_data(const glm::vec3& camera, const glm::vec2& dimensions,
                                  float height) {
  std::vector<float> data;
  std::vector<GLuint> indices;

  auto add_vec3 = [&](const glm::vec3& v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
    data.push_back(1.f);
  };

  auto distance = std::max(dimensions.x, dimensions.y);
  auto centre = camera + glm::vec3{0., height, 0.};
  add_vec3(centre + glm::vec3{-distance, 0, -distance});
  add_vec3(centre + glm::vec3{-distance, 0, distance});
  add_vec3(centre + glm::vec3{distance, 0, -distance});
  add_vec3(centre + glm::vec3{distance, 0, distance});
  indices.push_back(0);
  indices.push_back(2);
  indices.push_back(1);
  indices.push_back(1);
  indices.push_back(2);
  indices.push_back(3);

  glo::VertexData result{data, indices, GL_DYNAMIC_DRAW};
  result.enable_attribute(0, 4, 0, 0);
  return result;
}

}  // anonymous

WorldRenderer::WorldRenderer()
: world_program_{"world",
                 {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                 {"world_fragment", GL_FRAGMENT_SHADER, shaders::world_fragment}}
, material_program_{"material",
                    {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                    {"material_fragment", GL_FRAGMENT_SHADER, shaders::material_fragment}}
, light_program_{"light",
                 {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                 {"light_fragment", GL_FRAGMENT_SHADER, shaders::light_fragment}}
, fog_program_{"fog",
               {"fog_vertex", GL_VERTEX_SHADER, shaders::fog_vertex},
               {"fog_fragment", GL_FRAGMENT_SHADER, shaders::fog_fragment}} {}

void WorldRenderer::render(const Renderer& renderer, const glm::vec3& camera,
                           const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const {
  auto idimensions = renderer.framebuffer_dimensions();
  if (!world_buffer_ || world_buffer_->dimensions() != idimensions) {
    world_buffer_.reset(new glo::Framebuffer{idimensions});
    // World position buffer.
    world_buffer_->add_colour_buffer(/* high-precision RGB */ true);
    // World normal buffer.
    world_buffer_->add_colour_buffer(/* high-precision RGB */ true);
    // World material buffer.
    world_buffer_->add_colour_buffer(/* RGBA */ false);
    // World depth buffer.
    world_buffer_->add_depth_stencil_buffer();

    material_buffer_.reset(new glo::Framebuffer{idimensions});
    // Normal buffer.
    material_buffer_->add_colour_buffer(/* high-precision RGB */ true);
    // Colour buffer.
    material_buffer_->add_colour_buffer(/* RGBA */ false);

    world_buffer_->check_complete();
    material_buffer_->check_complete();
  }

  // Not sure what exact values we need for z-planes to be correct, this should do for now.
  glm::vec2 dimensions = idimensions;
  auto camera_distance =
      static_cast<float>(std::max(kTileSize, 2 * std::max(idimensions.x, idimensions.y)));
  auto ortho = glm::ortho(dimensions.x / 2, -dimensions.x / 2, -dimensions.y / 2, dimensions.y / 2,
                          -camera_distance, camera_distance);

  glm::vec3 up{0.f, 1.f, 0.f};
  glm::vec3 camera_direction{1.f, 1.f, -1.f};
  auto look_at = glm::lookAt(camera_direction, {}, up);

  // Do panning in screen-space to preserve pixels.
  glm::vec3 screen_space_translation = glm::round(look_at * glm::vec4{camera, 1.f});
  auto panning = glm::translate({}, -screen_space_translation);
  auto camera_matrix = ortho * panning * look_at;
  auto pixel_height = 1.f / look_at[1][1];
  renderer.set_dither_translation(-glm::ivec2{screen_space_translation});

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  {
    auto draw = world_buffer_->draw();
    glViewport(0, 0, idimensions.x, idimensions.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = world_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
    glUniform1f(program.uniform("frame"), static_cast<float>(renderer.frame()));
    renderer.set_simplex3_uniforms(program);
    generate_world_data(tile_map, pixel_height).draw();
  }

  renderer.set_default_render_states();
  {
    auto draw = material_buffer_->draw();
    glViewport(0, 0, idimensions.x, idimensions.y);

    auto program = material_program_.use();
    program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
    program.uniform_texture("world_buffer_normal", world_buffer_->colour_textures()[1]);
    program.uniform_texture("world_buffer_material", world_buffer_->colour_textures()[2]);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    glUniform1f(program.uniform("frame"), static_cast<float>(renderer.frame()));
    renderer.set_simplex3_uniforms(program);
    renderer.draw_quad();
  }

  auto light_position = camera + glm::vec3{0.f, 48.f, 0.f};
  {
    // Should be converted to draw the lights as individuals quads in a single draw call.
    auto program = light_program_.use();
    program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
    program.uniform_texture("material_buffer_normal", material_buffer_->colour_textures()[0]);
    program.uniform_texture("material_buffer_colour", material_buffer_->colour_textures()[1]);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    glUniform3fv(program.uniform("light_world"), 1, glm::value_ptr(light_position));
    glUniform1f(program.uniform("light_intensity"), 1.f);
    renderer.draw_quad();
  }

  {
    // Copy over the depth value so we can do forward rendering into the final buffer.
    auto read = world_buffer_->read();
    glBlitFramebuffer(0, 0, idimensions.x, idimensions.y, 0, 0, idimensions.x, idimensions.y,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Fog must be rendered in separate draw calls for transparency.
  auto render_fog = [&](float height, const glm::vec4 fog_colour) {
    auto program = fog_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
    renderer.set_simplex3_uniforms(program);
    glUniform4fv(program.uniform("fog_colour"), 1, glm::value_ptr(fog_colour));
    glUniform3fv(program.uniform("light_world"), 1, glm::value_ptr(light_position));
    glUniform1f(program.uniform("light_intensity"), 1.f);
    glUniform1f(program.uniform("frame"), static_cast<float>(renderer.frame()));
    generate_fog_data(camera, renderer.framebuffer_dimensions(), height).draw();
  };

  render_fog(-1.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .125});
  render_fog(-.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .25});
  render_fog(.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .25});
  render_fog(1.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .125});
}

}  // ::world
}  // ::gloam
