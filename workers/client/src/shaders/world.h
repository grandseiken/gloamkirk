#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_WORLD_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string world_vertex = R"""(
uniform mat4 camera_matrix;

layout(location = 0) in vec4 world_position;
layout(location = 1) in vec4 world_normal;
layout(location = 2) in float material;

flat out vec4 vertex_normal;
smooth out vec4 vertex_world;
smooth out float vertex_material;

void main()
{
  vertex_material = material;
  vertex_world = world_position;
  vertex_normal = world_normal;
  gl_Position = camera_matrix * world_position;
}
)""";

const std::string world_fragment = simplex3 + R"""(
flat in vec4 vertex_normal;
smooth in vec4 vertex_world;
smooth in float vertex_material;

layout(location = 0) out vec3 gbuffer_world;
layout(location = 1) out vec3 gbuffer_normal;
layout(location = 2) out vec4 gbuffer_colour;

void main()
{
  vec3 colour = vertex_material < 0.5 ?
      vec3(.0625, .5, .25) : vec3(.375, .375, .5);

  gbuffer_world = vec3(vertex_world);
  gbuffer_normal = vec3(vertex_normal);
  gbuffer_colour = vec4(colour, 0.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
