#ifndef GLOAM_WORKERS_MASTER_SRC_WORLDGEN_MAP_BUILDER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLDGEN_MAP_BUILDER_H
#include <glm/vec2.hpp>
#include <schema/chunk.h>
#include <cstdint>
#include <vector>

namespace gloam {
namespace master {
namespace worldgen {
class Random;
class RoomBuilder;

struct RoomType {
  const RoomBuilder* room_builder;
  std::uint32_t weight;
};

enum class MapType {
  GRASSLAND,
};

struct MapData {
  MapType map_type;
  glm::ivec2 dimensions;
  std::vector<schema::Tile> tiles;
};

schema::Tile::Terrain default_terrain(MapType map_type);

class MapBuilder {
public:
  MapBuilder(MapType type, const glm::ivec2& dimensions, std::int32_t base_height);
  void build(const std::vector<RoomType>& room_registry, Random& random);
  const MapData& data() const;

private:
  MapData map_data_;
  std::int32_t base_height_;
};

}  // ::worldgen
}  // ::master
}  // ::gloam

#endif