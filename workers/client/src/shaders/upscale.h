#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_UPSCALE_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_UPSCALE_H
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string upscale_vertex = R"""(
layout(location = 0) in vec4 position;
smooth out vec2 vertex_position;

void main()
{
  vertex_position = (1. + position.xy) / 2.;
  gl_Position = position;
}
)""";

const std::string upscale_fragment = R"""(
uniform sampler2D source_framebuffer;

smooth in vec2 vertex_position;
out vec4 output_colour;

void main()
{
  output_colour = texture(source_framebuffer, vertex_position);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
