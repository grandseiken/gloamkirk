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

void main()
{
  gl_Position = camera_matrix * position;
}
)""";

const std::string world_fragment = simplex3 + R"""(
out vec4 output_colour;

void main()
{
  output_colour = vec4(.0625, .5, .25, 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
