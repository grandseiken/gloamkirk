#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_LIGHT_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_LIGHT_H
#include "workers/client/src/shaders/common.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string light_fragment = gamma + R"""(
uniform sampler2D gbuffer_world;
uniform sampler2D gbuffer_normal;
uniform sampler2D gbuffer_colour;

uniform vec2 dimensions;
uniform vec3 light_world;
uniform float light_intensity;

out vec4 output_colour;

const float ambient_undirected = 1. / 128.;
const float ambient_intensity = 1. / 32.;
const vec3 ambient_direction = vec3(-1., -4., 2.);

float distance_factor(vec3 a, vec3 b)
{
  vec3 d = (b - a) / 32.;
  return pow(1. / dot(d, d), 1. / 8.);
}

float angle_factor(vec3 direction, vec3 normal)
{
  return clamp(dot(normalize(-direction), normal), 0., 1.);
}

void main()
{
  vec3 world = texture(gbuffer_world, gl_FragCoord.xy / dimensions).xyz;
  vec3 normal = texture(gbuffer_normal, gl_FragCoord.xy / dimensions).xyz;
  vec3 colour = texture(gbuffer_colour, gl_FragCoord.xy / dimensions).rgb;

  float light = ambient_undirected +
      ambient_intensity * angle_factor(ambient_direction, normal) +
      light_intensity * distance_factor(light_world, world) *
      (.25 + .75 * angle_factor(world - light_world, normal));

  vec3 final_colour = colour * clamp(light, 0., 1.);
  output_colour = vec4(final_colour, 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
