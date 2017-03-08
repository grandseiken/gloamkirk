#ifndef GLOAM_COMMON_SRC_COMMON_MATH_H
#define GLOAM_COMMON_SRC_COMMON_MATH_H
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

}  // ::common
}  // ::gloam

#endif
