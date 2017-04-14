#ifndef GLOAM_WORKERS_MASTER_SRC_WORLDGEN_MAP_BUILDER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLDGEN_MAP_BUILDER_H
#include <glm/vec2.hpp>
#include <schema/chunk.h>
#include <vector>

namespace gloam {
namespace master {
namespace worldgen {

enum class MapType {
  GRASSLAND,
};

struct MapData {
  MapType map_type;
  glm::ivec2 dimensions;
  std::vector<schema::Tile> tiles;
};

}  // ::worldgen
}  // ::master
}  // ::gloam

#endif