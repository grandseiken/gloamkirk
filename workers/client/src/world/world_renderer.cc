#include "workers/client/src/world/world_renderer.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/entity.h"
#include "workers/client/src/shaders/fog.h"
#include "workers/client/src/shaders/light.h"
#include "workers/client/src/shaders/world.h"
#include "workers/client/src/world/vertex_data.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

namespace gloam {
namespace world {
namespace {

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
  auto max_tile_size = std::max(kTileSize.y, std::max(kTileSize.x, kTileSize.z));
  auto camera_distance =
      std::max(max_tile_size, static_cast<float>(2 * std::max(dimensions.x, dimensions.y)));

  auto panning = glm::translate({}, -screen_space_translation(camera));
  auto ortho = glm::ortho(dimensions.x / 2.f, -dimensions.x / 2.f, -dimensions.y / 2.f,
                          dimensions.y / 2.f, -camera_distance, camera_distance);
  return ortho * panning * look_at_matrix();
}

}  // anonymous

WorldRenderer::WorldRenderer(const ModeState& mode_state)
: antialias_level_{1, mode_state.antialiasing ? 2 : 1}
, protrusion_program_{"protrusion",
                      {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                      {"protrusion_fragment", GL_FRAGMENT_SHADER, shaders::protrusion_fragment}}
, world_program_{"world",
                 {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                 {"world_fragment", GL_FRAGMENT_SHADER, shaders::world_fragment}}
, material_program_{"material",
                    {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                    {"material_fragment", GL_FRAGMENT_SHADER, shaders::material_fragment}}
, entity_program_{"entity",
                  {"entity_vertex", GL_VERTEX_SHADER, shaders::entity_vertex},
                  {"entity_fragment", GL_FRAGMENT_SHADER, shaders::entity_fragment}}
, light_program_{"light",
                 {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                 {"light_fragment", GL_FRAGMENT_SHADER, shaders::light_fragment}}
, fog_program_{"fog",
               {"fog_vertex", GL_VERTEX_SHADER, shaders::fog_vertex},
               {"fog_fragment", GL_FRAGMENT_SHADER, shaders::fog_fragment}} {}

void WorldRenderer::render(const Renderer& renderer, std::uint64_t frame,
                           const glm::vec3& camera_in, const std::vector<Light>& lights_in,
                           const std::vector<glm::vec3>& positions_in,
                           const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const {
  auto camera = kTileSize * camera_in;
  auto lights = lights_in;
  for (auto& light : lights) {
    light.world *= kTileSize;
  }
  auto positions = positions_in;
  for (auto& position : positions) {
    position *= kTileSize;
  }

  auto dimensions = renderer.framebuffer_dimensions();
  auto aa_dimensions = antialias_level_ * dimensions;
  auto protrusion_dimensions = dimensions + 2 * glm::ivec2{kPixelLayers};
  auto protrusion_aa_dimensions = antialias_level_ * protrusion_dimensions;

  if (!world_buffer_ || world_buffer_->dimensions() != dimensions) {
    create_framebuffers(aa_dimensions, protrusion_aa_dimensions);
  }
  renderer.set_dither_translation(-glm::ivec2{screen_space_translation(camera)});
  auto pixel_height = 1.f / look_at_matrix()[1][1] / antialias_level_.y;

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  {
    auto draw = protrusion_buffer_->draw();
    glViewport(0, 0, protrusion_aa_dimensions.x, protrusion_aa_dimensions.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = protrusion_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false,
                       glm::value_ptr(camera_matrix(camera, protrusion_dimensions)));
    glUniform1f(program.uniform("frame"), static_cast<float>(frame));
    renderer.set_simplex3_uniforms(program);
    generate_world_data(tile_map, camera_matrix(camera, protrusion_dimensions), false, pixel_height,
                        antialias_level_)
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
                 glm::value_ptr(glm::vec2{protrusion_aa_dimensions}));
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(glm::vec2{aa_dimensions}));
    generate_world_data(tile_map, camera_matrix(camera, dimensions), true, pixel_height,
                        antialias_level_)
        .draw();
  }

  renderer.set_default_render_states();
  {
    auto draw = material_buffer_->draw();
    glViewport(0, 0, aa_dimensions.x, aa_dimensions.y);

    // Render environment using the material program.
    material_buffer_->set_draw_buffers({1, 2});
    {
      auto program = material_program_.use();
      program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
      program.uniform_texture("world_buffer_normal", world_buffer_->colour_textures()[1]);
      program.uniform_texture("world_buffer_geometry", world_buffer_->colour_textures()[2]);
      program.uniform_texture("world_buffer_material", world_buffer_->colour_textures()[3]);
      glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(glm::vec2{aa_dimensions}));
      glUniform1f(program.uniform("frame"), static_cast<float>(frame));
      renderer.set_simplex3_uniforms(program);
      renderer.draw_quad();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    // Temporary, until we decide how to do character / object art.
    material_buffer_->set_draw_buffers({0, 1, 2});
    auto program = entity_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false,
                       glm::value_ptr(camera_matrix(camera, dimensions)));
    generate_entity_data(positions).draw();
  }

  auto render_lit_scene = [&] {
    renderer.set_default_render_states();
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
    std::vector<float> light_spreads;
    for (std::size_t i = 0; i < light_count; ++i) {
      light_worlds.push_back(lights[i].world.x);
      light_worlds.push_back(lights[i].world.y);
      light_worlds.push_back(lights[i].world.z);
      light_intensities.push_back(lights[i].intensity);
      light_spreads.push_back(lights[i].spread);
    }
    glUniform3fv(program.uniform("light_world"), static_cast<GLsizei>(light_count),
                 light_worlds.data());
    glUniform1fv(program.uniform("light_intensity"), static_cast<GLsizei>(light_count),
                 light_intensities.data());
    glUniform1fv(program.uniform("light_spread"), static_cast<GLsizei>(light_count),
                 light_spreads.data());
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
  // Rendering of unlit but anti-aliased objects (shadows, perhaps?) would go here.

  // Finally, downsample into the output buffer and render final / transparent effects which don't
  // need to be anti-aliased. If there's no composition buffer, we're already in the final buffer
  // and there's no downsampling.
  if (composition_buffer_) {
    auto read = composition_buffer_->read();
    glBlitFramebuffer(0, 0, aa_dimensions.x, aa_dimensions.y, 0, 0, dimensions.x, dimensions.y,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBlitFramebuffer(0, 0, aa_dimensions.x, aa_dimensions.y, 0, 0, dimensions.x, dimensions.y,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
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
    glUniform1f(program.uniform("frame"), static_cast<float>(frame));
    renderer.set_simplex3_uniforms(program);
    generate_fog_data(camera, 2 * dimensions, height).draw();
  };

  render_fog(-1.5f * kTileSize.y, glm::vec4{.5, .5, .5, .125});
  render_fog(-.5f * kTileSize.y, glm::vec4{.5, .5, .5, .25});
  render_fog(.5f * kTileSize.y, glm::vec4{.5, .5, .5, .25});
  render_fog(1.5f * kTileSize.y, glm::vec4{.5, .5, .5, .125});
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
  // World geometry buffer.
  world_buffer_->add_colour_buffer(/* RGBA */ false);
  // World material buffer.
  world_buffer_->add_colour_buffer(/* RGBA */ false);
  world_buffer_->add_depth_stencil_buffer();
  world_buffer_->set_draw_buffers({0, 1, 2, 3});
  world_buffer_->check_complete();

  // The material buffer is for the material pass which renders the colour and normal of the scene
  // using the information stored in previous buffers.
  material_buffer_.reset(new glo::Framebuffer{aa_dimensions});
  // Also has a copy of the world position buffer, so that we can render into it at the same time.
  material_buffer_->add_colour_buffer(world_buffer_->colour_textures().front());
  // Normal buffer.
  material_buffer_->add_colour_buffer(/* high-precision RGB */ true);
  // Colour buffer.
  material_buffer_->add_colour_buffer(/* RGBA */ false);
  material_buffer_->add_depth_stencil_buffer(*world_buffer_->depth_stencil_texture());
  material_buffer_->check_complete();

  // Finally the composition buffer renders the scene with lighting. After the composition stage
  // we downsample into the final output buffer and render effects like fog that don't benefit
  // from anti-aliasing. If anti-aliasing is disabled, there's no need for it.
  if (antialias_level_.x > 1 || antialias_level_.y > 1) {
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
