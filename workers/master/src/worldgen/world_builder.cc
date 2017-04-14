#include "world_builder.h"

namespace gloam {
namespace master {
namespace worldgen {

Random::Random(std::size_t seed) : generator{seed} {}

bool Random::get_bool() {
  return get_int(2) == 0;
}

std::int32_t Random::get_int(std::uint32_t max) {
  std::uniform_int_distribution<std::uint64_t> distribution{0, static_cast<std::uint64_t>(max) - 1};
  return static_cast<std::uint32_t>(distribution(generator));
}

std::int32_t Random::get_int(std::int32_t min, std::int32_t max) {
  auto range = static_cast<std::int64_t>(max) - static_cast<std::int64_t>(min);
  std::uniform_int_distribution<std::uint64_t> distribution{0,
                                                            static_cast<std::uint64_t>(range) - 1};
  auto value = static_cast<std::int64_t>(distribution(generator)) + static_cast<std::int64_t>(min);
  return static_cast<std::int32_t>(value);
}

}  // ::worldgen
}  // ::master
}  // ::gloam