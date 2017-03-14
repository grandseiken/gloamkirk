#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_LIGHT_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_LIGHT_H
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string light = R"""(
float distance_factor(vec3 a, vec3 b)
{
  vec3 d = (b - a) / 32.;
  return pow(1. / dot(d, d), 1. / 16.);
}

float cutoff_factor(vec3 a, vec3 b, float spread)
{
  float cutoff = 128.;
  float cutoff_sq = cutoff * cutoff;
  vec3 d = b - a;
  return 1. - clamp((dot(d, d) - spread * cutoff_sq / 2.) / (spread * cutoff_sq), 0., 1.);
}

float angle_factor(vec3 direction, vec3 normal)
{
  return clamp(dot(normalize(-direction), normal), 0., 1.);
}
)""";

const std::string light_fragment = tonemap + light + R"""(
uniform sampler2D world_buffer_position;
uniform sampler2D material_buffer_normal;
uniform sampler2D material_buffer_colour;

const int kLightMax = 8;

uniform vec2 dimensions;
uniform vec3 light_world[kLightMax];
uniform float light_intensity[kLightMax];
uniform float light_spread[kLightMax];
uniform int light_count;

out vec4 output_colour;

const float ambient_undirected = 1. / 32.;
const float ambient_intensity = 1. / 16.;
const vec3 ambient_direction = vec3(-1., -4., 2.);

void main()
{
  vec2 texture_coords = gl_FragCoord.xy / dimensions;
  vec3 world = texture(world_buffer_position, texture_coords).xyz;
  vec3 normal = texture(material_buffer_normal, texture_coords).xyz;
  vec3 colour = texture(material_buffer_colour, texture_coords).rgb;

  float total = ambient_undirected +
      ambient_intensity * angle_factor(ambient_direction, normal);

  for (int i = 0; i < light_count; ++i) {
    total += light_intensity[i] * distance_factor(light_world[i], world) *
        cutoff_factor(light_world[i], world, light_spread[i]) *
        (.25 + .75 * angle_factor(world - light_world[i], normal));
  }

  // Pretty hacky HDR: isn't applied consistently with fog, or anything.
  vec3 final_colour = colour * reinhard_tonemap(total);
  output_colour = vec4(final_colour, 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
