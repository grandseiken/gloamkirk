#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_POST_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_POST_H
#include "workers/client/src/shaders/common.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

#define POST_STRINGIF0(x) #x
#define POST_STRINGIFY(x) POST_STRINGIF0(x)
#define POST_CONSTANTS const int a_dither_res = 256;
POST_CONSTANTS

const std::string post_fragment = gamma + POST_STRINGIFY(POST_CONSTANTS) R"""(
uniform sampler2D source_framebuffer;
uniform sampler2D dither_matrix;
uniform vec2 dimensions;
uniform vec2 translation;

out vec4 output_colour;

// How much to mix in dithering versus true colouring.
const float dither_mix = 1.;
// How many quantization levels per colour.
const int colours_per_channel = 16;
const float quantize_div = 1. / (colours_per_channel - 1);

vec3 matrix_lookup(vec2 coord)
{
  return vec3(texture(dither_matrix, coord / a_dither_res));
}

vec3 floor_div(vec3 v)
{
  return v - mod(v, quantize_div);
}

vec3 linear_dither(vec3 value, vec3 dither)
{
  return floor_div(value + dither * quantize_div);
}

vec3 gamma_correct_dither(vec3 value, vec3 dither)
{
  vec3 a = floor_div(value);
  vec3 b = a + quantize_div;
  vec3 correct_ratio = 1 -
      ((gamma_apply(b) - gamma_apply(value)) /
       (gamma_apply(b) - gamma_apply(a)));
  return floor_div(a + (correct_ratio + dither) * quantize_div);
}

void main()
{
  vec3 value = vec3(texture(source_framebuffer, gl_FragCoord.xy / dimensions));
  vec3 dither = matrix_lookup(gl_FragCoord.xy + translation);

  output_colour = vec4(mix(value, gamma_correct_dither(value, dither), dither_mix), 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
