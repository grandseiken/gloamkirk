#ifndef GLOAM_CLIENT_SRC_SHADERS_TITLE_H
#define GLOAM_CLIENT_SRC_SHADERS_TITLE_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string title_vertex = R"""(
layout(location = 0) in vec4 position;

void main()
{
  gl_Position = position;
}
)""";

const std::string title_fragment = simplex3 + R"""(
uniform float frame;
uniform float random_seed;
uniform vec2 dimensions;
uniform vec2 title_dimensions;
uniform sampler2D title_texture;

out vec4 output_colour;

void main()
{
  vec2 frag = gl_FragCoord.xy;
  vec3 fog_seed = vec3(random_seed, 0., 0.) +
      vec3(frag.xy, 0.) + vec3(frame) / vec3(8., 32., 8.);
  float n =
      simplex3(fog_seed / 1024.) / 2. +
      simplex3(fog_seed / 512.) / 1. +
      simplex3(fog_seed / 256.) / 2. +
      simplex3(fog_seed / 128.) / 4. +
      simplex3(fog_seed / 64.) / 8. +
      simplex3(fog_seed / 32.) / 16. +
      simplex3(fog_seed / 16.) / 32. +
      simplex3(fog_seed / 8.) / 64. +
      simplex3(fog_seed / 4.) / 64. +
      simplex3(fog_seed / 2.) / 64.;
  float fog_value = (n + .5) / 4.;

  vec2 max_texture_scale = (dimensions - vec2(60., 180.)) / title_dimensions;
  float texture_scale = min(max_texture_scale.x, max_texture_scale.y);
  vec2 scaled_dimensions = texture_scale * title_dimensions;
  vec2 border = (dimensions - scaled_dimensions) / vec2(2., 6.);
  border.y = min(border.y, border.x);
  vec2 texture_coords = frag - vec2(border.x, dimensions.y - border.y - scaled_dimensions.y);

  vec3 value = vec3(clamp(fog_value, 0., 1.));
  if (all(greaterThanEqual(texture_coords, vec2(0., 0.))) &&
      all(lessThan(texture_coords, scaled_dimensions))) {
    vec2 sample_coords = texture_coords / scaled_dimensions;
    sample_coords.y = 1. - sample_coords.y;

    vec4 texture_value = texture(title_texture, sample_coords);

    float fog_mix = 1. - clamp((fog_value - .125) * 8., 0., 1.);
    float frame_mix = clamp((frame - 64.) / 256., 0., 1.);
    value = mix(value, texture_value.rgb, texture_value.a * fog_mix * frame_mix);
  }
  output_colour = vec4(value.rg, pow(value.b, .875), 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
