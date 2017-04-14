#ifndef GLOAM_WORKERS_MASTER_SRC_WORLDGEN_WORLD_BUILDER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLDGEN_WORLD_BUILDER_H
#include <cstdlib>
#include <random>

namespace gloam {
namespace master {
namespace worldgen {

class Random {
public:
  Random(std::size_t seed);

  bool get_bool();
  std::int32_t get_int(std::uint32_t max);
  std::int32_t get_int(std::int32_t min, std::int32_t max);

private:
  std::mt19937 generator;
};

}  // ::worldgen
}  // ::master
}  // ::gloam

#endif