#include "room_builder.h"
#include "map_builder.h"

namespace gloam {
namespace master {
namespace worldgen {

bool xy_swapped(Symmetry symmetry) {
  return symmetry == Symmetry::kLeftCw || symmetry == Symmetry::kLeftCcw ||
      symmetry == Symmetry::kRightCw || symmetry == Symmetry::kRightCcw;
}

MapType MapWriter::map_type() const {
  return data_.map_type;
}

MapWriter::MapWriter(MapData& data, const glm::ivec2& target_origin,
                     const glm::ivec2& target_dimensions, Symmetry symmetry,
                     std::int32_t base_height)
: data_{data}
, target_origin_{target_origin}
, target_dimensions_{target_dimensions}
, symmetry_{symmetry}
, base_height_{base_height} {}

glm::ivec2 MapWriter::dimensions() const {
  return xy_swapped(symmetry_) ? glm::ivec2{target_dimensions_.y, target_dimensions_.x}
                               : target_dimensions_;
}

void MapWriter::set_tile(const glm::ivec2& position, schema::Tile::Terrain terrain,
                         std::int32_t height) const {
  auto& tile = find_tile(position);
  tile.set_terrain(terrain);
  tile.set_height(base_height_ + height);
}

schema::Tile::Terrain MapWriter::get_terrain(const glm::ivec2& position) const {
  return find_tile(position).terrain();
}

std::int32_t MapWriter::get_height(const glm::ivec2& position) const {
  return find_tile(position).height() - base_height_;
}

schema::Tile& MapWriter::find_tile(const glm::ivec2& position) const {
  static schema::Tile default_tile = {schema::Tile::Terrain::kGrass, 0, schema::Tile::Ramp::kNone};

  auto v = position;
  if (xy_swapped(symmetry_)) {
    v = glm::ivec2{v.y, v.x};
  }
  if (v.x < 0 || v.y < 0 || v.x >= target_dimensions_.x || v.y >= target_dimensions_.y) {
    return default_tile;
  }
  if (symmetry_ == Symmetry::kDownCcw || symmetry_ == Symmetry::kUpCw ||
      symmetry_ == Symmetry::kRightCw || symmetry_ == Symmetry::kRightCcw) {
    v.x = target_dimensions_.x - 1 - v.x;
  }
  if (symmetry_ == Symmetry::kUpCw || symmetry_ == Symmetry::kUpCcw ||
      symmetry_ == Symmetry::kLeftCw || symmetry_ == Symmetry::kRightCcw) {
    v.y = target_dimensions_.y - 1 - v.y;
  }
  auto map_position = target_origin_ + v;
  return data_.tiles[map_position.x + map_position.y * data_.dimensions.x];
}

RoomBuilder::RoomBuilder(std::uint32_t flags, const glm::ivec2& min_dimensions,
                         const glm::ivec2& max_dimensions)
: flags_{static_cast<RoomBuilderFlags>(flags)}
, min_dimensions_{min_dimensions}
, max_dimensions_{max_dimensions} {}

RoomBuilderFlags RoomBuilder::flags() const {
  return flags_;
}

const glm::ivec2& RoomBuilder::min_dimensions() const {
  return min_dimensions_;
}

const glm::ivec2& RoomBuilder::max_dimensions() const {
  return max_dimensions_;
}

}  // ::worldgen
}  // ::master
}  // ::gloam
