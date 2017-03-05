#ifndef GLOAM_CLIENT_SRC_SHADERS_TEXT_H
#define GLOAM_CLIENT_SRC_SHADERS_TEXT_H
#include <string>

namespace gloam {
namespace shaders {
namespace {

/* clang-format off */
const std::uint32_t text_widths[128] = {
    0, 0,  0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,  0,  0,
    0, 0,  0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0,  0,  0,
    2, 4,  7, 13, 8,  8,  8,  3,  4,  4, 8, 8, 2, 6,  2,  4,
    8, 4,  7, 6,  9,  7,  7,  7,  8,  8, 3, 3, 5, 6,  5,  8,
    9, 9,  8, 8,  8,  8,  8,  8,  8,  4, 4, 8, 7, 14, 8,  9,
    9, 10, 9, 8,  10, 10, 10, 15, 10, 9, 8, 4, 4, 4,  5,  6,
    3, 8,  9, 7,  8,  7,  8,  8,  11, 4, 4, 7, 6, 15, 10, 8,
    9, 9,  8, 8,  7,  9,  9,  14, 8,  7, 7, 5, 2, 5,  7,  0,
};
/* clang-format on */

const std::uint32_t text_border = 2;
const std::uint32_t text_height = 14;
const std::uint32_t text_size = 16;

const std::string text_vertex = R"""(
uniform vec2 framebuffer_dimensions;
uniform vec2 text_dimensions;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texture_coords;

smooth out vec2 vertex_texture_coords;

void main()
{
  vertex_texture_coords = texture_coords / text_dimensions;
  gl_Position = vec4(2. * (position / framebuffer_dimensions) - 1., 1., 1.);
  gl_Position.y = -gl_Position.y;
}
)""";

const std::string text_fragment = R"""(
uniform sampler2D text_texture;
uniform vec4 text_colour;

smooth in vec2 vertex_texture_coords;
out vec4 output_colour;

void main()
{
  output_colour = vec4(
      text_colour.rgb,
      text_colour.a * texture(text_texture, vertex_texture_coords).a);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
