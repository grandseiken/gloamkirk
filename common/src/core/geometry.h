#ifndef GLOAM_COMMON_SRC_CORE_GEOMETRY_H
#define GLOAM_COMMON_SRC_CORE_GEOMETRY_H
#include <glm/vec2.hpp>
#include <array>

namespace gloam {
namespace core {
constexpr const std::size_t kBoxCorners = 4;

// Diagonal-axis-aligned collision box, with origin at centre-bottom.
struct Box {
  // Half-width on each diagonal axis.
  float radius;
};

inline std::array<glm::vec2, kBoxCorners> get_corners(const Box& box) {
  return {glm::vec2{-box.radius, 0.f}, glm::vec2{0.f, box.radius}, glm::vec2{box.radius, 0.f},
          glm::vec2{0.f, -box.radius}};
}

inline std::array<glm::vec2, kBoxCorners> get_corners(const glm::ivec2& coord) {
  glm::vec2 v = coord;
  return {v, v + glm::vec2{0., 1.}, v + glm::vec2{1., 1.}, v + glm::vec2{1., 0.}};
}

}  // ::core
}  // ::gloam

#endif
