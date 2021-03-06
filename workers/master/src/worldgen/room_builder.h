#ifndef GLOAM_WORKERS_MASTER_SRC_WORLDGEN_ROOM_BUILDER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLDGEN_ROOM_BUILDER_H
#include <glm/vec2.hpp>
#include <schema/chunk.h>
#include <cstdint>

namespace gloam {
namespace master {
namespace worldgen {
class Random;
struct MapData;
enum class MapType;

// Possible orientations for a MapWriter.
enum class Symmetry {
  kDownCw,
  kDownCcw,
  kUpCw,
  kUpCcw,
  kLeftCw,
  kLeftCcw,
  kRightCw,
  kRightCcw,
};

// Room builder settings.
enum RoomBuilderFlags : std::uint32_t {
  kNone = 0,
  // A room that can be laid over other rooms.
  kCanOverlay = 1,
  // A room that cannot have other rooms laid over it.
  kExclusive = 2,
  // A special room.
  kSpecial = 4,
};

bool xy_swapped(Symmetry symmetry);

class MapWriter {
public:
  MapWriter(MapData& data, const glm::ivec2& target_origin, const glm::ivec2& target_dimensions,
            Symmetry symmetry, std::int32_t base_height);

  MapType map_type() const;
  glm::ivec2 dimensions() const;
  void set_tile(const glm::ivec2& position, schema::Tile::Terrain terrain,
                std::int32_t height) const;
  schema::Tile::Terrain get_terrain(const glm::ivec2& position) const;
  std::int32_t get_height(const glm::ivec2& position) const;

private:
  schema::Tile& find_tile(const glm::ivec2& position) const;

  MapData& data_;
  glm::ivec2 target_origin_;
  glm::ivec2 target_dimensions_;
  Symmetry symmetry_;
  std::int32_t base_height_;
};

class RoomBuilder {
public:
  RoomBuilder(std::uint32_t flags, const glm::ivec2& min_dimensions,
              const glm::ivec2& max_dimensions);

  RoomBuilderFlags flags() const;
  const glm::ivec2& min_dimensions() const;
  const glm::ivec2& max_dimensions() const;

  virtual void build(MapWriter& writer, Random& random) const = 0;

private:
  RoomBuilderFlags flags_;
  glm::ivec2 min_dimensions_;
  glm::ivec2 max_dimensions_;
};

}  // ::worldgen
}  // ::master
}  // ::gloam

#endif