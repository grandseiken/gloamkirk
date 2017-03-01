#ifndef GLOAM_CLIENT_SRC_SHADERS_FOG_H
#define GLOAM_CLIENT_SRC_SHADERS_FOG_H
#include "workers/client/src/shaders/simplex.h"
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string fog_vertex = R"""(
layout(location = 0) in vec4 position;

void main()
{
  gl_Position = position;
}
)""";

const std::string fog_fragment = simplex3 + R"""(
uniform float frame;

out vec4 output_colour;

void main()
{
  vec3 seed = vec3(gl_FragCoord.xy, 0.) +
      vec3(frame) / vec3(8., 32., 8.);
  float n =
      simplex3(seed / 512.) +
      simplex3(seed / 256.) / 2. +
      simplex3(seed / 128.) / 4. +
      simplex3(seed / 64.) / 8. +
      simplex3(seed / 32.) / 16. +
      simplex3(seed / 16.) / 32. +
      simplex3(seed / 8.) / 64. +
      simplex3(seed / 4.) / 128. +
      simplex3(seed / 2.) / 256.;
  float v = clamp((n / 2. + .5) / 2., 0., 1.);
  output_colour = vec4(v, v, v, 1.);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif