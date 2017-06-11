#ifndef GLOAM_COMMON_SRC_COMMON_MATH_H
#define GLOAM_COMMON_SRC_COMMON_MATH_H
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <schema/chunk.h>
#include <cmath>
#include <cstdint>

namespace gloam {
namespace common {

template <typename T>
T euclidean_mod(T n, T div) {
  auto abs_n = std::abs(n);
  auto abs_div = std::abs(div);
  return (n >= 0 ? n : n + ((abs_n % abs_div ? 1 : 0) + abs_n / abs_div) * abs_div) % abs_div;
}

template <typename T>
T euclidean_div(T n, T div) {
  bool sign = (n < 0) == (div < 0);
  auto t = (n < 0 ? -(1 + n) : n) / std::abs(div);
  return (div < 0 ? 1 : 0) + (sign ? t : -(1 + t));
}

inline glm::vec2 get_xz(const glm::vec3& xyz) {
  return {xyz.x, xyz.z};
}

inline glm::vec3 from_xz(const glm::vec2& xz, float y) {
  return {xz.x, y, xz.y};
}

inline glm::ivec2 ramp_direction(schema::Tile::Ramp ramp_type) {
  switch (ramp_type) {
  case schema::Tile::Ramp::kRight:
    return {1, 0};
  case schema::Tile::Ramp::kLeft:
    return {-1, 0};
  case schema::Tile::Ramp::kUp:
    return {0, 1};
  case schema::Tile::Ramp::kDown:
    return {0, -1};
  default:
    return {0, 0};
  }
}

}  // ::common
}  // ::gloam

#endif
