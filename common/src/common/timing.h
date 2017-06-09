#ifndef GLOAM_COMMON_SRC_COMMON_TIMING_H
#define GLOAM_COMMON_SRC_COMMON_TIMING_H
#include <chrono>
#include <cstdint>

namespace gloam {
namespace common {
namespace {
// Timing setup. 60 frames per second; 20 sync steps per second (for rapidly-changing components).
// Should really be 16ms every third frame, but monitor vsync seems to actually lock to exactly
// 17ms?
constexpr const std::chrono::duration<std::uint64_t, std::ratio<1, 1000>> kTickDuration{17};
constexpr const std::uint32_t kTicksPerSync = 3;

// Speeds are measured in tiles per sync tick.
const float kPlayerSpeed = 4.5f / 32.f;
const float kGravity = 1.5f / 8.f;
}  // anonymous
}  // ::common
}  // ::gloam

#endif
