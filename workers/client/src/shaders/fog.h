#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_FOG_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_FOG_H
#include "workers/client/src/shaders/light.h"
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string fog_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec4 world_position;

smooth out vec4 vertex_world;

void main()
{
  vertex_world = world_position;
  gl_Position = camera_matrix * world_position;
}
)""";

const std::string fog_fragment = light + simplex3 + R"""(
uniform vec4 fog_colour;
uniform vec3 light_world;
uniform float light_intensity;
uniform float frame;

smooth in vec4 vertex_world;

out vec4 output_colour;

void main() {
  vec3 world = vec3(vertex_world);
  vec3 fog_seed = world + vec3(frame) / vec3(8., 32., 8.);
  float n =
      simplex3(fog_seed / 1024.) / 1. +
      simplex3(fog_seed / 512.) / 2. +
      simplex3(fog_seed / 256.) / 1. +
      simplex3(fog_seed / 128.) / 2. +
      simplex3(fog_seed / 64.) / 4. +
      simplex3(fog_seed / 32.) / 8. +
      simplex3(fog_seed / 16.) / 16. +
      simplex3(fog_seed / 8.) / 32. +
      simplex3(fog_seed / 4.) / 64. +
      simplex3(fog_seed / 2.) / 32.;

  float fog_value = clamp((n + 1.) / 2., 0., 1.);
  float light_value = distance_factor(light_world, world) *
      cutoff_factor(light_world, world, light_intensity);

  output_colour = fog_colour;
  output_colour.a *= fog_value * (1. - light_value / 2.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
