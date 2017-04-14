#include "world_builder.h"
#include "common/src/common/hashes.h"
#include "common/src/common/math.h"
#include "map_builder.h"
#include <schema/chunk.h>
#include <unordered_set>

namespace gloam {
namespace master {
namespace worldgen {

Random::Random(std::size_t seed) : generator{static_cast<unsigned int>(seed)} {}

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

WorldBuilder::WorldBuilder(WorldType type) : type_{type} {}

std::vector<glm::ivec2> WorldBuilder::get_chunks(std::int32_t chunk_size) const {
  std::unordered_set<glm::ivec2> result;
  for (const auto& map : maps_) {
    auto min = map.origin;
    auto max = map.origin + map.builder->data().dimensions;
    for (auto y = common::euclidean_div(min.y, chunk_size);
         y <= common::euclidean_div(max.y - 1, chunk_size); ++y) {
      for (auto x = common::euclidean_div(min.x, chunk_size);
           x <= common::euclidean_div(max.x - 1, chunk_size); ++x) {
        result.insert({x, y});
      }
    }
  }
  return {result.begin(), result.end()};
}

schema::ChunkData WorldBuilder::get_chunk_data(std::int32_t chunk_size,
                                               const glm::ivec2& chunk) const {
  schema::ChunkData data{chunk_size, chunk.x, chunk.y, {}};
  return data;
}

}  // ::worldgen
}  // ::master
}  // ::gloam