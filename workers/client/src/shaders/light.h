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

float cutoff_factor(vec3 a, vec3 b, float intensity)
{
  float cutoff = 256.;
  float cutoff_sq = cutoff * cutoff;
  vec3 d = b - a;
  return 1. - clamp((dot(d, d) -  cutoff_sq) / (intensity * cutoff_sq), 0., 1.);
}

float angle_factor(vec3 direction, vec3 normal)
{
  return clamp(dot(normalize(-direction), normal), 0., 1.);
}
)""";

const std::string light_fragment = light + R"""(
uniform sampler2D world_buffer_position;
uniform sampler2D material_buffer_normal;
uniform sampler2D material_buffer_colour;

uniform vec2 dimensions;
uniform vec3 light_world;
uniform float light_intensity;

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

  float light = ambient_undirected +
      ambient_intensity * angle_factor(ambient_direction, normal) +
      light_intensity * distance_factor(light_world, world) *
      cutoff_factor(light_world, world, light_intensity) *
      (.25 + .75 * angle_factor(world - light_world, normal));

  vec3 final_colour = colour * clamp(light, 0., 1.);
  output_colour = vec4(final_colour, 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
