#include "common/src/collision/collision.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace gloam {
namespace collision {
namespace {
const float kTolerance = 1.f / 256;

std::uint32_t coords(float v) {
  return static_cast<std::uint32_t>(glm::floor(v));
}

glm::ivec2 coords(const glm::vec2& v) {
  return glm::floor(v);
}

// Outward-facing normal of clockwise edge.
glm::vec2 outward_normal(const Edge& edge) {
  auto edge_v = edge.b - edge.a;
  return {-edge_v.y, edge_v.x};
}

// Whether clockwise edge could possible impede the given projection.
bool can_collide(const Edge& edge, const glm::vec2& projection) {
  return glm::dot(outward_normal(edge), projection) < 0.f;
}

float cross_2d(const glm::vec2& a, const glm::vec2& b) {
  return a.x * b.y - a.y * b.x;
}

float project(const Edge& edge, const glm::vec2& position, const glm::vec2& projection) {
  // Given v(t) = position + t * projection and w(u) = edge.a + u * (edge.b - edge.a),
  // finds t and u with v(t) = w(u).
  auto edge_v = edge.b - edge.a;
  auto d = cross_2d(edge_v, projection);
  if (!d) {
    return 1.f;
  }
  auto t = cross_2d(edge_v, edge.a - position) / d;
  auto u = cross_2d(projection, edge.a - position) / d;
  if (u < 0 || u > 1 || t < -kTolerance) {
    return 1.f;
  }
  return std::max(0.f, std::min(1.f, t));
}

}  // anonymous

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
    height_map_[pair.first] = coords.z;
    first = false;
  }

  auto has_edge = [&](std::int32_t height, const glm::ivec2& from, const glm::ivec2& to) {
    auto from_it = tile_map.find(from);
    auto to_it = tile_map.find(to);
    return from_it != tile_map.end() && height >= from_it->second.height() &&
        (to_it == tile_map.end() || height < to_it->second.height());
  };

  for (auto height = min.z; height < max.z; ++height) {
    auto& layer = layers_[height];

    // X-axis-aligned edges.
    for (auto y = min.y; y <= max.y; ++y) {
      std::int32_t scan_start = 0;
      bool scanning = false;

      for (auto x = min.x; x <= 1 + max.x; ++x) {
        bool edge = x <= max.x && has_edge(height, {x, y}, {x, y - 1});
        if (edge && !scanning) {
          scan_start = x;
        } else if (!edge && scanning) {
          layer.edges.push_back({{scan_start, y}, {x, y}});
        }
        if ((scanning = edge)) {
          layer.tile_lookup[{x, y}].push_back(layer.edges.size());
        }
      }

      for (auto x = max.x; 1 + x >= min.x; --x) {
        bool edge = x >= min.x && has_edge(height, {x, y}, {x, y + 1});
        if (edge && !scanning) {
          scan_start = x;
        } else if (!edge && scanning) {
          layer.edges.push_back({{1 + scan_start, 1 + y}, {1 + x, 1 + y}});
        }
        if ((scanning = edge)) {
          layer.tile_lookup[{x, y}].push_back(layer.edges.size());
        }
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
          layer.edges.push_back({{1 + x, scan_start}, {1 + x, y}});
        }
        if ((scanning = edge)) {
          layer.tile_lookup[{x, y}].push_back(layer.edges.size());
        }
      }

      for (auto y = max.y; 1 + y >= min.y; --y) {
        bool edge = y >= min.y && has_edge(height, {x, y}, {x - 1, y});
        if (edge && !scanning) {
          scan_start = y;
        } else if (!edge && scanning) {
          layer.edges.push_back({{x, 1 + scan_start}, {x, 1 + y}});
        }
        if ((scanning = edge)) {
          layer.tile_lookup[{x, y}].push_back(layer.edges.size());
        }
      }
    }
  }
}

float Collision::project_xz(const Box& box, const glm::vec3& position,
                            const glm::vec2& projection) const {
  auto layer_it = layers_.find(coords(position.y));
  if (layer_it == layers_.end()) {
    return 1.f;
  }
  const auto& layer = layer_it->second;

  static const std::size_t kCorners = 4;
  glm::vec2 xz = {position.x, position.z};
  glm::vec2 corners[kCorners] = {xz + glm::vec2{-box.radius, 0.f}, xz + glm::vec2{box.radius, 0.f},
                                 xz + glm::vec2{0.f, -box.radius}, xz + glm::vec2{0.f, box.radius}};
  glm::ivec2 min;
  glm::ivec2 max;
  bool first = true;
  for (std::size_t i = 0; i < kCorners; ++i) {
    if (first) {
      min = max = glm::ivec2{corners[i]};
    }
    min = glm::min(min, coords(corners[i] - kTolerance));
    max = glm::max(max, coords(corners[i] + kTolerance));
    min = glm::min(min, coords(corners[i] + projection - kTolerance));
    max = glm::max(max, coords(corners[i] + projection + kTolerance));
    first = false;
  }

  std::unordered_set<std::size_t> edges;
  for (std::int32_t y = min.y; y <= max.y; ++y) {
    for (std::int32_t x = min.x; x <= max.x; ++x) {
      auto it = layer.tile_lookup.find({x, y});
      if (it != layer.tile_lookup.end()) {
        edges.insert(it->second.begin(), it->second.end());
      }
    }
  }

  for (const auto& edge_index : edges) {
    const auto& edge = layer.edges[edge_index];
    if (!can_collide(edge, projection)) {
      continue;
    }
  }
}

}  // ::collision
}  // ::gloam
