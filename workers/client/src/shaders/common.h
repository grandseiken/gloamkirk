#ifndef GLOAM_WORKERS_CLIENT_SRC_SHADERS_COMMON_H
#define GLOAM_WORKERS_CLIENT_SRC_SHADERS_COMMON_H
#include <string>

namespace gloam {
namespace shaders {
namespace {

const std::string gamma = R"""(
const float gamma_value = 2.2;
const float inverse_value = 1. / gamma_value;

vec3 gamma_apply(vec3 v)
{
  return vec3(pow(v.r, gamma_value),
              pow(v.g, gamma_value),
              pow(v.b, gamma_value));
}

vec3 gamma_inverse(vec3 v)
{
  return vec3(pow(v.r, inverse_value),
              pow(v.g, inverse_value),
              pow(v.b, inverse_value));
}
)""";

const std::string quad_vertex = R"""(
layout(location = 0) in vec4 position;

void main()
{
  gl_Position = position;
}
)""";

const std::string quad_colour_fragment = R"""(
uniform vec4 colour;
out vec4 output_colour;

void main()
{
  output_colour = colour;
}
)""";

const std::string quad_texture_fragment = R"""(
uniform vec2 dimensions;
uniform sampler2D source_texture;

out vec4 output_colour;

void main()
{
  output_colour = texture(source_texture, gl_FragCoord.xy / dimensions);
}
)""";

}  // anonymous
}  // ::shaders
}  // ::gloam

#endif
