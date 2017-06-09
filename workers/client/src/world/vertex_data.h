#ifndef GLOAM_WORKERS_CLIENT_SRC_WORLD_VERTEX_DATA_H
#define GLOAM_WORKERS_CLIENT_SRC_WORLD_VERTEX_DATA_H
#include "workers/client/src/glo.h"
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>
#include <schema/chunk.h>

namespace gloam {
namespace world {
namespace {
// Tile size in pixels.
const glm::vec3 kTileSize = {32, 32, 32};
// Number of layers for voxel-based surface rendering.
const std::int32_t kPixelLayers = 8;
}  // anonymous namespace

glo::VertexData generate_world_data(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map,
const glm::mat4& camera_matrix, bool world_pass, float pixel_height, const glm::ivec2&
antialias_level);

glo::VertexData generate_entity_data(const std::vector<glm::vec3>& positions);

glo::VertexData generate_fog_data(const glm::vec3& camera, const glm::vec2& dimensions,
float height);
}  // ::world
}  // ::gloam

#endif
