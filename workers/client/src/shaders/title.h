#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_TITLE_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_TITLE_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string title_fragment = simplex3 + R"""(
uniform float fade;
uniform float frame;
uniform float random_seed;
uniform float title_alpha;
uniform vec2 dimensions;
uniform vec2 scaled_dimensions;
uniform vec2 border;
uniform sampler2D title_texture;

out vec4 output_colour;

void main()
{
  vec2 frag = gl_FragCoord.xy;
  vec3 fog_seed = vec3(random_seed, 0., 0.) +
      vec3(frag.xy - dimensions.xy / 2., 0.) + vec3(frame) / vec3(8., 32., 8.);
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
  float fog_value = n / 6. + 1. / 6.;
  vec3 value = vec3(clamp(fog_value, 0., 1.));

  vec2 texture_coords = frag - vec2(border.x, dimensions.y - border.y - scaled_dimensions.y);
  if (all(greaterThanEqual(texture_coords, vec2(0., 0.))) &&
      all(lessThan(texture_coords, scaled_dimensions))) {
    vec2 sample_coords = texture_coords / scaled_dimensions;
    sample_coords.y = 1. - sample_coords.y;

    vec4 texture_value = texture(title_texture, sample_coords);

    float fog_mix = 1. - clamp((fog_value - .125) * 8., 0., 1.);
    value = mix(value, texture_value.rgb, texture_value.a * fog_mix * title_alpha);
  }
  output_colour = vec4(value.rg, pow(value.b, .875), 1.);
  output_colour *= fade;
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
