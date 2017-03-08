#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string world_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 colour;

flat out vec4 vertex_colour;

void main()
{
  vertex_colour = colour;
  gl_Position = camera_matrix * position;
}
)""";

const std::string world_fragment = simplex3 + R"""(
uniform vec4 colour;
out vec4 output_colour;

flat in vec4 vertex_colour;

void main()
{
  output_colour = vertex_colour;
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
