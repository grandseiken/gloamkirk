#include "common/src/collision/collision.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace gloam {
namespace collision {

void Collision::update(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) {
  layers_.clear();
  glm::ivec3 min;
  glm::ivec3 max;

  bool first = true;
  for (const auto& pair : tile_map) {
    glm::ivec3 coords = {pair.first.x, pair.first.y, pair.second.height()};
    if (first) {
      min = max = coords;
    }
    min = glm::min(min, coords);
    max = glm::max(max, coords);
  }

  auto has_edge = [&](std::uint32_t height, const glm::ivec2& from, const glm::ivec2& to) {
    auto from_it = tile_map.find(from);
    auto to_it = tile_map.find(to);
    return from_it != tile_map.end() && height >= from_it->second.height() &&
        (to_it == tile_map.end() || height < to_it->second.height());
  };

  for (auto height = min.z; height < max.z; ++height) {
    // X-axis-aligned edges.
    for (auto y = min.y; y <= max.y; ++y) {
      std::int32_t scan_start = 0;
      bool scanning = false;

      for (auto x = min.x; x <= 1 + max.x; ++x) {
        bool edge = x <= max.x && has_edge(height, {x, y}, {x, y - 1});
        if (edge && !scanning) {
          scan_start = x;
        } else if (!edge && scanning) {
          layers_[height].edges.push_back({{scan_start, y}, {x, y}});
        }
        scanning = edge;
      }

      for (auto x = max.x; 1 + x >= min.x; --x) {
        bool edge = x >= min.x && has_edge(height, {x, y}, {x, y + 1});
        if (edge && !scanning) {
          scan_start = x;
        } else if (!edge && scanning) {
          layers_[height].edges.push_back({{1 + scan_start, 1 + y}, {1 + x, 1 + y}});
        }
        scanning = edge;
      }
    }

    // Y-axis-aligned edges.
    for (auto x = min.x; x <= max.x; ++x) {
      std::int32_t scan_start = 0;
      bool scanning = false;

      for (auto y = min.y; y <= 1 + max.y; ++y) {
        bool edge = y <= max.y && has_edge(height, {x, y}, {x + 1, y});
        if (edge && !scanning) {
          scan_start = y;
        } else if (!edge && scanning) {
          layers_[height].edges.push_back({{1 + x, scan_start}, {1 + x, y}});
        }
        scanning = edge;
      }

      for (auto y = max.y; 1 + y >= min.y; --y) {
        bool edge = y >= min.y && has_edge(height, {x, y}, {x - 1, y});
        if (edge && !scanning) {
          scan_start = y;
        } else if (!edge && scanning) {
          layers_[height].edges.push_back({{x, 1 + scan_start}, {x, 1 + y}});
        }
        scanning = edge;
      }
    }
  }
}

}  // ::collision
}  // ::gloam
