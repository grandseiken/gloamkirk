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
// TODO: should these be a graphics quality settings?
const std::int32_t kPixelLayers = 8;
const glm::ivec2 kAntialiasLevel = {1, 2};

bool is_visible(const glm::mat4& camera_matrix, const std::vector<glm::vec3>& vertices) {
  bool xlo = false;
  bool xhi = false;
  bool ylo = false;
  bool yhi = false;
  bool zlo = false;
  bool zhi = false;

  for (const auto& v : vertices) {
    auto s = camera_matrix * glm::vec4{v, 1.f};
    xlo = xlo || s.x >= -1.f;
    xhi = xhi || s.x <= 1.f;
    ylo = ylo || s.y >= -1.f;
    yhi = yhi || s.y <= 1.f;
    zlo = zlo || s.z >= -1.f;
    zhi = zhi || s.z <= 1.f;
  }
  return xlo && xhi && ylo && yhi && zlo && zhi;
}

// TODO: should see if we can cache this between frames.
glo::VertexData generate_world_data(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map,
                                    const glm::mat4& camera_matrix, bool world_pass,
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

    std::vector<glm::vec3> vertices = {{min.x, height, min.y},
                                       {min.x, height, max.y},
                                       {max.x, height, min.y},
                                       {max.x, height, max.y}};
    auto all_vertices = vertices;
    for (const auto& v : vertices) {
      all_vertices.emplace_back(v + glm::vec3{0.f, kPixelLayers * pixel_height, 0.f});
    }

    bool visible = is_visible(camera_matrix, all_vertices);
    auto max_layer = world_pass ? kPixelLayers : 1;
    for (std::int32_t pixel_layer = 0; visible && pixel_layer < max_layer; ++pixel_layer) {
      auto world_height = pixel_layer * pixel_height;
      auto world_offset = glm::vec4{0.f, world_height, 0.f, 0.f};

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

      if (is_visible(
              camera_matrix,
              {{min.x, min.y, y}, {min.x, max.y, y}, {max.x, min.y, y}, {max.x, max.y, y}})) {
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
    }

    jt = tile_map.find(coord - glm::ivec2{1, 0});
    if (jt != tile_map.end()) {
      auto x = coord.x * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.y * kTileSize, next_height};
      max = glm::vec2{(1 + coord.y) * kTileSize, height};
      glm::vec4 side_normal = {jt->second.height() > pair.second.height() ? 1. : -1., 0., 0., 1.};

      if (is_visible(
              camera_matrix,
              {{x, min.y, min.x}, {x, min.y, max.x}, {x, max.y, min.x}, {x, max.y, max.x}})) {
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
  }

  glo::VertexData result{data, indices, GL_DYNAMIC_DRAW};
  // World position.
  result.enable_attribute(0, 4, 11, 0);
  // Vertex normals.
  result.enable_attribute(1, 4, 11, 4);
  // Material (material value, edge value, height value, ...?).
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

glm::mat4 look_at_matrix() {
  glm::vec3 up{0.f, 1.f, 0.f};
  glm::vec3 camera_direction{1.f, 1.f, -1.f};
  return glm::lookAt(camera_direction, {}, up);
}

glm::vec3 screen_space_translation(const glm::vec3& camera) {
  // We do panning in screen-space pixel-coordinates so that camera movement doesn't cause the
  // environment to glitch around.
  return glm::round(look_at_matrix() * glm::vec4{camera, 1.f});
}

glm::mat4 camera_matrix(const glm::vec3& camera, const glm::ivec2& dimensions) {
  // Not sure what exact values we need for z-planes to be correct. This should do for now.
  auto camera_distance =
      static_cast<float>(std::max(kTileSize, 2 * std::max(dimensions.x, dimensions.y)));

  auto panning = glm::translate({}, -screen_space_translation(camera));
  auto ortho = glm::ortho(dimensions.x / 2.f, -dimensions.x / 2.f, -dimensions.y / 2.f,
                          dimensions.y / 2.f, -camera_distance, camera_distance);
  return ortho * panning * look_at_matrix();
}

}  // anonymous

WorldRenderer::WorldRenderer()
: protrusion_program_{"protrusion",
                      {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                      {"protrusion_fragment", GL_FRAGMENT_SHADER, shaders::protrusion_fragment}}
, world_program_{"world",
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

void WorldRenderer::render(const Renderer& renderer, const glm::vec3& camera_in,
                           const std::vector<Light>& lights_in,
                           const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const {
  auto camera = static_cast<float>(kTileSize) * camera_in;
  auto lights = lights_in;
  for (auto& light : lights) {
    light.world *= static_cast<float>(kTileSize);
  }

  auto dimensions = renderer.framebuffer_dimensions();
  auto aa_dimensions = kAntialiasLevel * dimensions;
  auto protrusion_dimensions = 2 * dimensions;
  // Need pixel-perfect sampling from a dimensions-sized centred box, so have to do adjust this.
  protrusion_dimensions -= dimensions % 2;

  if (!world_buffer_ || world_buffer_->dimensions() != dimensions) {
    create_framebuffers(aa_dimensions, protrusion_dimensions);
  }
  renderer.set_dither_translation(-glm::ivec2{screen_space_translation(camera)});
  auto pixel_height = 1.f / look_at_matrix()[1][1];

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  {
    auto draw = protrusion_buffer_->draw();
    glViewport(0, 0, protrusion_dimensions.x, protrusion_dimensions.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = protrusion_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false,
                       glm::value_ptr(camera_matrix(camera, protrusion_dimensions)));
    glUniform1f(program.uniform("frame"), static_cast<float>(renderer.frame()));
    renderer.set_simplex3_uniforms(program);
    generate_world_data(
        tile_map, camera_matrix(camera, dimensions + 2 * glm::ivec2{kPixelLayers, kPixelLayers}),
        false, pixel_height)
        .draw();
  }

  {
    auto draw = world_buffer_->draw();
    glViewport(0, 0, aa_dimensions.x, aa_dimensions.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = world_program_.use();
    program.uniform_texture("protrusion_buffer", protrusion_buffer_->colour_textures()[0]);
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false,
                       glm::value_ptr(camera_matrix(camera, dimensions)));
    glUniform2fv(program.uniform("protrusion_buffer_dimensions"), 1,
                 glm::value_ptr(glm::vec2{protrusion_dimensions}));
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(glm::vec2{dimensions}));
    generate_world_data(tile_map, camera_matrix(camera, dimensions), true, pixel_height).draw();
  }

  renderer.set_default_render_states();
  {
    auto draw = material_buffer_->draw();
    glViewport(0, 0, aa_dimensions.x, aa_dimensions.y);

    auto program = material_program_.use();
    program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
    program.uniform_texture("world_buffer_normal", world_buffer_->colour_textures()[1]);
    program.uniform_texture("world_buffer_material", world_buffer_->colour_textures()[2]);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(glm::vec2{aa_dimensions}));
    glUniform1f(program.uniform("frame"), static_cast<float>(renderer.frame()));
    renderer.set_simplex3_uniforms(program);
    renderer.draw_quad();
  }

  auto render_lit_scene = [&] {
    glViewport(0, 0, aa_dimensions.x, aa_dimensions.y);
    glClear(GL_COLOR_BUFFER_BIT);

    // Should be converted to draw the lights as individuals quads in a single draw call.
    auto program = light_program_.use();
    program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
    program.uniform_texture("material_buffer_normal", material_buffer_->colour_textures()[0]);
    program.uniform_texture("material_buffer_colour", material_buffer_->colour_textures()[1]);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(glm::vec2{aa_dimensions}));

    auto light_count = std::min<std::size_t>(8u, lights.size());
    std::vector<float> light_worlds;
    std::vector<float> light_intensities;
    for (std::size_t i = 0; i < light_count; ++i) {
      light_worlds.push_back(lights[i].world.x);
      light_worlds.push_back(lights[i].world.y);
      light_worlds.push_back(lights[i].world.z);
      light_intensities.push_back(lights[i].intensity);
    }
    glUniform3fv(program.uniform("light_world"), static_cast<GLsizei>(light_count),
                 light_worlds.data());
    glUniform1fv(program.uniform("light_intensity"), static_cast<GLsizei>(light_count),
                 light_intensities.data());
    glUniform1i(program.uniform("light_count"), static_cast<GLsizei>(light_count));
    renderer.draw_quad();

    // Copy over the depth value so we can do forward rendering into the composite buffer.
    auto read = world_buffer_->read();
    glBlitFramebuffer(0, 0, aa_dimensions.x, aa_dimensions.y, 0, 0, aa_dimensions.x,
                      aa_dimensions.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  };

  if (composition_buffer_) {
    auto draw = composition_buffer_->draw();
    render_lit_scene();
  } else {
    render_lit_scene();
  }

  // Finally, downsample into the output buffer and render final / transparent effects which don't
  // need to be anti-aliased. If there's no composition buffer, we're already in the final buffer
  // and there's no downsampling.
  if (composition_buffer_) {
    auto read = composition_buffer_->read();
    glBlitFramebuffer(0, 0, aa_dimensions.x, aa_dimensions.y, 0, 0, dimensions.x, dimensions.y,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBlitFramebuffer(0, 0, aa_dimensions.x, aa_dimensions.y, 0, 0, dimensions.x, dimensions.y,
                      GL_DEPTH_BUFFER_BIT, GL_LINEAR);
  }

  glViewport(0, 0, dimensions.x, dimensions.y);
  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Fog must be rendered in separate draw calls for transparency.
  auto render_fog = [&](float height, const glm::vec4 fog_colour) {
    auto program = fog_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false,
                       glm::value_ptr(camera_matrix(camera, dimensions)));
    glUniform4fv(program.uniform("fog_colour"), 1, glm::value_ptr(fog_colour));
    glUniform3fv(program.uniform("light_world"), 1, glm::value_ptr(lights.front().world));
    glUniform1f(program.uniform("light_intensity"), lights.front().intensity);
    glUniform1f(program.uniform("frame"), static_cast<float>(renderer.frame()));
    renderer.set_simplex3_uniforms(program);
    generate_fog_data(camera, 2 * dimensions, height).draw();
  };

  render_fog(-1.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .125});
  render_fog(-.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .25});
  render_fog(.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .25});
  render_fog(1.5f * static_cast<float>(kTileSize), glm::vec4{.5, .5, .5, .125});
}

void WorldRenderer::create_framebuffers(const glm::ivec2& aa_dimensions,
                                        const glm::ivec2& protrusion_dimensions) const {
  // World height buffer. This is the first pass where we render the pixel height (protrusion) of
  // surfaces into a temporary buffer, low-resolution buffer (the dimensions are doubled just so
  // we can sample past the edge of the screen). Not scaled up for antialiasing since it makes for
  // a blurry result.
  protrusion_buffer_.reset(new glo::Framebuffer{protrusion_dimensions});
  protrusion_buffer_->add_colour_buffer(/* high-precision RGB */ true);
  protrusion_buffer_->add_depth_stencil_buffer();
  protrusion_buffer_->check_complete();

  // The dimensions and scale of the remaining buffers are increased for antialiasing. The world
  // buffer is for the second pass. This is a very simple pass which uses the height buffer to
  // render many layers of pixels with depth-checking to construct the geometry.
  world_buffer_.reset(new glo::Framebuffer{aa_dimensions});
  // World position buffer.
  world_buffer_->add_colour_buffer(/* high-precision RGB */ true);
  // World normal buffer.
  world_buffer_->add_colour_buffer(/* high-precision RGB */ true);
  // World material buffer.
  world_buffer_->add_colour_buffer(/* RGBA */ false);
  world_buffer_->add_depth_stencil_buffer();
  world_buffer_->check_complete();

  // The material buffer is for the material pass which renders the colour and normal of the scene
  // using the information stored in previous buffers.
  material_buffer_.reset(new glo::Framebuffer{aa_dimensions});
  // Normal buffer.
  material_buffer_->add_colour_buffer(/* high-precision RGB */ true);
  // Colour buffer.
  material_buffer_->add_colour_buffer(/* RGBA */ false);
  material_buffer_->check_complete();

  // Finally the composition buffer renders the scene with lighting. After the composition stage
  // we downsample into the final output buffer and render effects like fog that don't benefit
  // from anti-aliasing. If anti-aliasing is disabled, there's no need for it.
  if (kAntialiasLevel.x > 1 || kAntialiasLevel.y > 1) {
    composition_buffer_.reset(new glo::Framebuffer{aa_dimensions});
    composition_buffer_->add_colour_buffer(/* RGBA */ false);
    composition_buffer_->add_depth_stencil_buffer();
    composition_buffer_->check_complete();
  } else {
    composition_buffer_.reset();
  }
}

}  // ::world
}  // ::gloam
