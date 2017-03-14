#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_ENTITY_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_ENTITY_H
#include "workers/client/src/shaders/common.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string entity_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec3 world_position;
layout(location = 1) in vec3 world_normal;
layout(location = 2) in vec3 world_colour;

flat out vec3 vertex_normal;
smooth out vec3 vertex_world;
smooth out vec3 vertex_colour;

void main()
{
  vertex_normal = world_normal;
  vertex_world = world_position;
  vertex_colour = world_colour;
  gl_Position = camera_matrix * vec4(world_position, 1.);
}
)""";

const std::string entity_fragment = R"""(
flat in vec3 vertex_normal;
smooth in vec3 vertex_world;
smooth in vec3 vertex_colour;

layout(location = 0) out vec3 material_buffer_position;
layout(location = 1) out vec3 material_buffer_normal;
layout(location = 2) out vec4 material_buffer_colour;

void main() {
  material_buffer_position = vertex_world;
  material_buffer_normal = vertex_normal;
  material_buffer_colour = vec4(vertex_colour, 0.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
