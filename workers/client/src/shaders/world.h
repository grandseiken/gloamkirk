#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string world_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec4 world_position;
layout(location = 1) in vec4 world_normal;
layout(location = 2) in float material;

flat out vec4 vertex_normal;
smooth out vec4 vertex_world;
smooth out float vertex_material;

void main()
{
  vertex_material = material;
  vertex_world = world_position;
  vertex_normal = world_normal;
  gl_Position = camera_matrix * world_position;
}
)""";

const std::string world_fragment = gamma + simplex3 + R"""(
uniform vec4 colour;
uniform vec4 light_world;
uniform float light_intensity;
out vec4 output_colour;

flat in vec4 vertex_normal;
smooth in vec4 vertex_world;
smooth in float vertex_material;

const float ambient_undirected = 1. / 64.;
const float ambient_intensity = 1. / 16.;
const vec4 ambient_direction = vec4(-1., -4., 2., 1.);

float square_distance(vec4 a, vec4 b)
{
  vec4 d = (b - a) / 32.;
  return dot(d, d);
}

float angle_factor(vec4 direction, vec4 normal)
{
  return clamp(dot(normalize(vec3(-direction)), vec3(normal)), 0., 1.);
}

void main()
{
  vec3 vertex_colour = vertex_material < 0.5 ?
      vec3(.0625, .5, .25) : vec3(.375, .375, .5);

  float light = ambient_undirected +
      ambient_intensity * angle_factor(ambient_direction, vertex_normal) +
      light_intensity * angle_factor(vertex_world - light_world, vertex_normal) /
      square_distance(light_world, vertex_world);

  vec3 final_colour = vertex_colour * clamp(light, 0., 1.);
  output_colour = vec4(gamma_inverse(final_colour), 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
