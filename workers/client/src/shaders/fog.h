#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_FOG_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_FOG_H
#include "workers/client/src/shaders/common.h"
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

const std::string fog_fragment = common + light + simplex3 + R"""(
uniform vec4 fog_colour;
uniform vec3 light_world;
uniform float light_intensity;
uniform float frame;

smooth in vec4 vertex_world;
smooth in vec4 vertex_screen;

out vec4 output_colour;

void main() {
  vec3 world = vec3(vertex_world);

  vec3 fog_seed =  + world + vec3(frame) / vec3(8., 32., 8.);

  vec4 n =
      simplex3_gradient(fog_seed * d1024) * d4 +
      simplex3_gradient(fog_seed * d512) * d2 +
      simplex3_gradient(fog_seed * d256) * d1 +
      simplex3_gradient(fog_seed * d128) * d2 +
      simplex3_gradient(fog_seed * d64) * d4 +
      simplex3_gradient(fog_seed * d32) * d8 +
      simplex3_gradient(fog_seed * d16) * d16 +
      simplex3_gradient(fog_seed * d8) * d8;

  vec3 u = vec3(1., 0., 0.);
  vec3 v = vec3(0., 0., 1.);
  vec3 up = vec3(0., 1., 0.);
  vec3 grad_normal = normalize(cross(u + up * dot(u, n.xyz), v + up * dot(v, n.xyz)));
  grad_normal.y = world.y < light_world.y ? grad_normal.y : -grad_normal.y;
  float incidence = angle_factor(world - light_world, grad_normal);

  float light_value = distance_factor(light_world, world) *
      cutoff_factor(light_world, world, light_intensity);

  vec3 output_rgb =
      clamp((1. + clamp(light_value, 0., 1.) * incidence) / 2., 0., 1.) *
      cutoff_factor(light_world, world, 8. * light_intensity) *
      (fog_colour.rgb + vec3(n.w) / 2.);

  output_colour = vec4(output_rgb.rgb,
      fog_colour.a * incidence * clamp(1. - light_value / 4., 0., 1.));
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
