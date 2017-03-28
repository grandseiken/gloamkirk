#ifndef GLOAM_COMMON_SRC_COMMON_TIMING_H
#define GLOAM_COMMON_SRC_COMMON_TIMING_H
#include <chrono>
#include <cstdint>

namespace gloam {
namespace common {
namespace {
// Timing setup. 60 frames per second; 20 sync steps per second (for rapidly-changing components).
constexpr const std::chrono::duration<std::uint64_t, std::ratio<1, 1000>> kSyncTickDuration{16};
constexpr const std::chrono::duration<std::uint64_t, std::ratio<1, 1000>> kTickDuration{17};
constexpr const std::uint32_t kTicksPerSync = 3;

// Speeds are measured in tiles per sync tick.
const float kPlayerSpeed = 4.5f / 32.f;
}  // anonymous
}  // ::common
}  // ::gloam

#endif
