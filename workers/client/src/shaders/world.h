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

const std::string protrusion_fragment = common + simplex3 + R"""(
uniform float frame;

flat in vec4 vertex_normal;
smooth in vec4 vertex_world;
smooth in vec3 vertex_material;

layout(location = 0) out vec3 protrusion_buffer;

void main()
{
  vec3 world = vec3(vertex_world);
  vec3 normal = vec3(vertex_normal);
  float material = vertex_material.r;
  float edge_value = vertex_material.g;

  float protrusion_value = 0.;
  if (normal.y >= 1.) {
    float s512 = simplex3(world * d512);
    float s256 = simplex3(world * d256);
    float s128 = simplex3(world * d128);
    float s64 = simplex3(world * d64);
    float s32 = simplex3(world * d32);
    float s16 = simplex3(world * d16);
    float s2 = simplex3(world * d2);
    float s1 = simplex3(world * d1);

    float value = d2 * s512 + d1 * s256 + d2 * s128 + d4 * s64 + d8 * s32 + d16 * s16;
    float detail_value = 4. * (1. + s32 + s16 + s2 + 2. * s1);

    protrusion_value =
        clamp(detail_value, 0., 16.) *
        clamp(edge_value + d2 + (value * 8.), 0., 1.);
  }
  protrusion_buffer = vec3(protrusion_value, 0., 0.);
}
)""";

const std::string world_fragment = R"""(
uniform sampler2D protrusion_buffer;
uniform vec2 protrusion_buffer_dimensions;
uniform vec2 dimensions;
uniform mat4 camera_matrix;

flat in vec4 vertex_normal;
smooth in vec4 vertex_world;
smooth in vec3 vertex_material;

layout(location = 0) out vec3 world_buffer_position;
layout(location = 1) out vec3 world_buffer_normal;
layout(location = 2) out vec4 world_buffer_material;

void main()
{
  float vertex_protrusion = vertex_material.b;
  vec4 base_world = vertex_world - vec4(0., vertex_protrusion, 0., 0.);
  vec2 pixel_coords = dimensions * ((camera_matrix * base_world).xy + 1.) / 2.;
  vec2 protrusion_pixel_coords = .5 + floor(pixel_coords) +
      (protrusion_buffer_dimensions - dimensions) / 2.;
  vec2 texture_coords = protrusion_pixel_coords / protrusion_buffer_dimensions;
  float world_protrusion = texture(protrusion_buffer, texture_coords).r;

  if (vertex_protrusion > world_protrusion) {
    discard;
  } else {
    world_buffer_position = vec3(vertex_world);
    world_buffer_normal = vec3(vertex_normal);
    world_buffer_material = vec4(vertex_material, 0.);
  }
}
)""";

const std::string material_fragment = common + simplex3 + R"""(
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
  float protrusion = texture(world_buffer_material, texture_coords).b;

  vec3 base_world = world - vec3(0., protrusion, 0.);
  // Perpendicular unit vectors in the plane.
  vec3 plane_u = normalize(normal.x == normal.y ?
      vec3(-normal.z, 0, normal.x) : vec3(-normal.y, normal.x, 0));
  vec3 plane_v = cross(normal, plane_u);

  vec3 colour = vec3(0.);
  vec3 grass_colour = vec3(3. / 16., 3. / 4., 1. / 2.);
  vec3 stone_colour = vec3(1. / 4., 1. / 2., 1. / 2.);

  float s512 = simplex3(base_world * d512);
  float s256 = simplex3(base_world * d256);
  float s128 = simplex3(base_world * d128);
  vec4 g64 = simplex3_gradient(base_world * d64);
  vec4 g32 = simplex3_gradient(base_world * d32);
  vec4 g16 = simplex3_gradient(base_world * d16);
  vec4 g8 = simplex3_gradient(base_world * d8);
  vec4 g2 = simplex3_gradient(base_world * d2);

  float value = d2 * s512 + d1 * s256 + d2 * s128 + d4 * g64.w + d8 * g32.w + d16 * g16.w;
  float mix_value = clamp(d4 * protrusion + edge_value + d2 + (value * 8.), 0., 1.);
  if (normal.y < 1.) {
    mix_value = 0.;
  }

  vec3 grass_normal = normalize(normal + d64 * vec3(g2.x, 0., g2.z));
  vec4 stone_value = d2 * g64 + d1 * g32 + d2 * g16 + d4 * g8;

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
