#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string world_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec4 world_position;
layout(location = 1) in vec4 world_normal;
layout(location = 2) in vec2 material;

flat out vec4 vertex_normal;
smooth out vec4 vertex_world;
smooth out vec2 vertex_material;

void main()
{
  vertex_material = material;
  vertex_world = world_position;
  vertex_normal = world_normal;
  gl_Position = camera_matrix * world_position;
}
)""";

const std::string world_fragment = simplex3 + R"""(
flat in vec4 vertex_normal;
smooth in vec4 vertex_world;
smooth in vec2 vertex_material;

layout(location = 0) out vec3 world_buffer_position;
layout(location = 1) out vec3 world_buffer_normal;
layout(location = 2) out vec4 world_buffer_material;

void main()
{
  world_buffer_position = vec3(vertex_world);
  world_buffer_normal = vec3(vertex_normal);
  world_buffer_material = vec4(vertex_material, 0., 0.);
}
)""";

const std::string material_fragment = simplex3 + R"""(
uniform sampler2D world_buffer_position;
uniform sampler2D world_buffer_normal;
uniform sampler2D world_buffer_material;
uniform vec2 dimensions;

layout(location = 0) out vec3 material_buffer_normal;
layout(location = 1) out vec4 material_buffer_colour;

void main() {
  vec2 texture_coords = gl_FragCoord.xy / dimensions;
  vec3 world = texture(world_buffer_position, texture_coords).xyz;
  vec3 normal = texture(world_buffer_normal, texture_coords).xyz;
  float edge_value = texture(world_buffer_material, texture_coords).r;
  float material = texture(world_buffer_material, texture_coords).g;

  // Perpendicular unit vectors in the plane.
  vec3 plane_u = normalize(normal.x == normal.y ?
      vec3(-normal.z, 0, normal.x) : vec3(-normal.y, normal.x, 0));
  vec3 plane_v = cross(normal, plane_u);

  vec3 colour = vec3(0.);
  if (material < 0.5) {
    float value =
        simplex3(world / 256.) / 2. +
        simplex3(world / 128.) / 4. +
        simplex3(world / 64.) / 8. +
        simplex3(world / 32.) / 16.;
        simplex3(world / 16.) / 32. +
        simplex3(world / 8.) / 64. +
        simplex3(world / 4.) / 128. +
        simplex3(world / 2.) / 256.;

    vec3 grass_colour = vec3(3. / 16., 3. / 4., 1. / 2.);
    vec3 stone_colour = vec3(1. / 4., 1. / 2., 1. / 2.);

    vec4 stone_value =
        simplex3_gradient(world / 64.) / 2. +
        simplex3_gradient(world / 32.) / 1. +
        simplex3_gradient(world / 16.) / 2. +
        simplex3_gradient(world / 8.) / 4.;

    float mix_value = clamp(2. * edge_value + clamp(.5 + (value * 8.), 0., 1.), 0., 1.);
    stone_colour *=
        (stone_value.w >= -1. / 8. && stone_value.w <= 1. / 8.)
        || mix_value > 0. ? .75 : 1.;
    if (mix_value > 0.) {
      stone_value.w = 1. / 8.;
    }

    vec3 stone_normal = normal;
    if (stone_value.w >= 1. / 16. && stone_value.w <= 3. / 16.) {
      stone_normal = normalize(cross(
        plane_u + normal * dot(plane_u, stone_value.xyz),
        plane_v + normal * dot(plane_v, stone_value.xyz)));
    } else if (stone_value.w >= -3. / 16. && stone_value.w <= -1. / 16.) {
      stone_normal = normalize(cross(
        plane_u + normal * dot(plane_u, -stone_value.xyz),
        plane_v + normal * dot(plane_v, -stone_value.xyz)));
    }

    colour = mix(stone_colour, grass_colour, clamp(mix_value * 2. - 1., 0., 1.));
    normal = normalize(mix(stone_normal, normal, clamp(.75 + mix_value, 0., 1.)));
  } else {
    vec3 seed = world;
    seed.y /= 8.;
    float value = (simplex3(seed / 32.) + simplex3(seed / 8.)) / 2.;

    const vec3 wall_colour = vec3(.65, .65, .75);
    colour = value * value < 1. / 32. ? wall_colour * .75 : wall_colour;
  }

  material_buffer_normal = normal;
  material_buffer_colour = vec4(colour, 0.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
