#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string material_common = R"""(
float material0_value(vec3 world) {
  return
      simplex3(world / 256.) / 2. +
      simplex3(world / 128.) / 4. +
      simplex3(world / 64.) / 8. +
      simplex3(world / 32.) / 16.;
      simplex3(world / 16.) / 32. +
      simplex3(world / 8.) / 64. +
      simplex3(world / 4.) / 128. +
      simplex3(world / 2.) / 256.;
}
)""";

const std::string world_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec4 world_position;
layout(location = 1) in vec4 world_normal;
layout(location = 2) in vec3 material;

flat out vec4 vertex_normal;
smooth out vec4 vertex_world;
smooth out vec3 vertex_material;

void main()
{
  vertex_material = material;
  vertex_world = world_position;
  vertex_normal = world_normal;
  gl_Position = camera_matrix * world_position;
}
)""";

const std::string world_fragment = simplex3 + material_common + R"""(
uniform float frame;

flat in vec4 vertex_normal;
smooth in vec4 vertex_world;
smooth in vec3 vertex_material;

layout(location = 0) out vec3 world_buffer_position;
layout(location = 1) out vec3 world_buffer_normal;
layout(location = 2) out vec4 world_buffer_material;

void main()
{
  vec3 world = vec3(vertex_world);
  vec3 normal = vec3(vertex_normal);
  float material = vertex_material.r;
  float edge_value = vertex_material.g;
  float height_value = vertex_material.b;

  vec3 base_world = world - vec3(0., height_value, 0.);
  if (material < .5) {
    float value = material0_value(base_world);
    float detail_value = 4. +
        4. * simplex3(base_world / 32.) +
        4. * simplex3(base_world / 8.) +
        6. * simplex3(base_world / 2.) +
        8. * simplex3(base_world / 1.);
    float height_here = clamp(detail_value, 0., 16.) *
        clamp(edge_value + .5 + (value * 8.), 0., 1.);

    if (height_value > height_here) {
      discard;
    }
  } else {
    if (height_value > 0.) {
      discard;
    }
  }

  world_buffer_position = world;
  world_buffer_normal = normal;
  world_buffer_material = vec4(vertex_material, 0.);
}
)""";

const std::string material_fragment = simplex3 + material_common + R"""(
uniform sampler2D world_buffer_position;
uniform sampler2D world_buffer_normal;
uniform sampler2D world_buffer_material;
uniform vec2 dimensions;
uniform float frame;

layout(location = 0) out vec3 material_buffer_normal;
layout(location = 1) out vec4 material_buffer_colour;

void main() {
  vec2 texture_coords = gl_FragCoord.xy / dimensions;
  vec3 world = texture(world_buffer_position, texture_coords).xyz;
  vec3 normal = texture(world_buffer_normal, texture_coords).xyz;
  float material = texture(world_buffer_material, texture_coords).r;
  float edge_value = texture(world_buffer_material, texture_coords).g;
  float height_value = texture(world_buffer_material, texture_coords).b;

  vec3 base_world = world - vec3(0., height_value, 0.);
  // Perpendicular unit vectors in the plane.
  vec3 plane_u = normalize(normal.x == normal.y ?
      vec3(-normal.z, 0, normal.x) : vec3(-normal.y, normal.x, 0));
  vec3 plane_v = cross(normal, plane_u);

  vec3 colour = vec3(0.);
  vec3 grass_colour = vec3(3. / 16., 3. / 4., 1. / 2.);
  vec3 stone_colour = vec3(1. / 4., 1. / 2., 1. / 2.);

  float value = material0_value(base_world);
  float mix_value = clamp(
      .25 * height_value + edge_value + .5 + (value * 8.), 0., 1.);
  if (normal.y < 1.) {
    mix_value = 0.;
  }

  vec4 grass_perturb =
      simplex3_gradient(base_world / 4.) / 128. +
      simplex3_gradient(base_world / 2.) / 128.;
  vec3 grass_normal = normalize(normal + vec3(grass_perturb.x, 0., grass_perturb.z));

  vec4 stone_value =
      simplex3_gradient(world / 64.) / 2. +
      simplex3_gradient(world / 32.) / 1. +
      simplex3_gradient(world / 16.) / 2. +
      simplex3_gradient(world / 8.) / 4.;

  stone_colour *=
      (stone_value.w >= -1. / 8. && stone_value.w <= 1. / 8.)
      || mix_value > 0. || edge_value > .75 ? .75 : 1.;
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
  normal = normalize(mix(
      mix(stone_normal, normal, clamp(.75 + mix_value, 0., 1.)),
      grass_normal, mix_value));

  material_buffer_normal = normal;
  material_buffer_colour = vec4(colour, 0.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
