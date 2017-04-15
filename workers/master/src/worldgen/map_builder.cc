#include "map_builder.h"

namespace gloam {
namespace master {
namespace worldgen {

schema::Tile::Terrain default_terrain(MapType map_type) {
  return map_type == MapType::GRASSLAND ? schema::Tile::Terrain::GRASS
                                        : schema::Tile::Terrain::GRASS;
}

MapBuilder::MapBuilder(MapType type, const glm::ivec2& dimensions, std::int32_t base_height)
: map_data_{type, dimensions, {}}, base_height_{base_height} {
  auto tile_count = static_cast<std::size_t>(dimensions.x * dimensions.y);
  map_data_.tiles.reserve(tile_count);
  for (std::size_t i = 0; i < tile_count; ++i) {
    map_data_.tiles.push_back({default_terrain(type), base_height, schema::Tile::Ramp::NONE});
  }
}

void MapBuilder::build(const std::vector<RoomType>&, Random&) {}

const MapData& MapBuilder::data() const {
  return map_data_;
}

}  // ::worldgen
}  // ::master
}  // ::gloam