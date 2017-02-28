#ifndef GLOAM_CLIENT_SRC_SHADERS_POST_H
#define GLOAM_CLIENT_SRC_SHADERS_POST_H
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string post_vertex = R"""(
layout(location = 0) in vec4 position;
smooth out vec2 vertex_position;

void main()
{
  vertex_position = (1. + position.xy) / 2.;
  gl_Position = position;
}
)""";

#define POST_STRINGIF0(x) #x
#define POST_STRINGIFY(x) POST_STRINGIF0(x)
#define POST_CONSTANTS const int a_dither_res = 256;
POST_CONSTANTS

const std::string post_fragment = POST_STRINGIFY(POST_CONSTANTS) R"""(
uniform sampler2D source_framebuffer;
uniform sampler2D dither_matrix;
uniform float frame;

smooth in vec2 vertex_position;
out vec4 output_colour;

// Whether dithering moves around (based on directions above), and
// whether it is separated by colour.
const bool dither_monochrome = true;
// How much to mix in dithering versus true colouring.
const float dither_mix = 1.;
// How many quantization levels per colour.
const int colours_per_channel = 12;
const float quantize_div = 1. / (colours_per_channel - 1);
const float gamma_value = 2.2;

// Make sure no direction aligns exactly with an axis.
const float base_dir = .2;
const float pi = 3.1415926536;
const vec2 r_dir = vec2(sin(base_dir), cos(base_dir));
const vec2 g_dir = vec2(sin(base_dir + 2. * pi / 3.), cos(base_dir + 2. * pi / 3.));
const vec2 b_dir = vec2(sin(base_dir + 4. * pi / 3.), cos(base_dir + 4. * pi / 3.));

vec2 make_coord(vec2 coord, vec2 offset)
{
  return (coord + (dither_monochrome ? vec2(0.) : offset)) / a_dither_res;
}

vec3 matrix_lookup(vec2 coord, vec2 r, vec2 g, vec2 b)
{
  return vec3(
      texture(dither_matrix, make_coord(coord, r)));
}

vec3 floor_div(vec3 v)
{
  return v - mod(v, quantize_div);
}

vec3 linear_dither(vec3 value, vec3 dither)
{
  return floor_div(value +
      mix(vec3(.5 * quantize_div), dither * quantize_div, dither_mix));
}

vec3 gamma_write(vec3 v)
{
  return vec3(pow(v.r, gamma_value),
              pow(v.g, gamma_value),
              pow(v.b, gamma_value));
}

vec3 gamma_correct_dither(vec3 value, vec3 dither)
{
  vec3 a = floor_div(value);
  vec3 b = a + quantize_div;
  vec3 correct_ratio = 1 -
      ((gamma_write(b) - gamma_write(value)) /
       (gamma_write(b) - gamma_write(a)));
  return floor_div(a + (correct_ratio + dither) * quantize_div);
}

void main()
{
  vec2 r_off = .05 * r_dir * frame;
  vec2 g_off = .07 * g_dir * frame;
  vec2 b_off = .11 * b_dir * frame;

  vec3 value = vec3(texture(source_framebuffer, vertex_position));
  vec3 dither = matrix_lookup(gl_FragCoord.xy, r_off, g_off, b_off);
  output_colour = vec4(gamma_correct_dither(value, dither), 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif