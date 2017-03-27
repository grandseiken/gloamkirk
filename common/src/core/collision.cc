#include "common/src/core/collision.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace gloam {
namespace core {
namespace {
const float kTolerance = 1.f / 256;
const float kToleranceSq = kTolerance * kTolerance;

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

// Whether clockwise edge could possibly impede the given projection.
bool can_collide(const Edge& edge, const glm::vec2& projection) {
  return glm::dot(outward_normal(edge), projection) < 0.f;
}

float cross_2d(const glm::vec2& a, const glm::vec2& b) {
  return a.x * b.y - a.y * b.x;
}

float project(const Edge& edge, const glm::vec2& origin, const glm::vec2& projection) {
  // Given v(t) = position + t * projection and w(u) = edge.a + u * (edge.b - edge.a),
  // finds t and u with v(t) = w(u).
  auto edge_v = edge.b - edge.a;
  auto d = cross_2d(edge_v, projection);
  if (!d) {
    return 1.f;
  }
  auto t = cross_2d(edge_v, edge.a - origin) / d;
  auto u = cross_2d(projection, edge.a - origin) / d;
  if (u < 0 || u > 1 || t * glm::dot(projection, projection) < -kToleranceSq) {
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
    height_map_[pair.first] = static_cast<float>(coords.z);
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
        scanning = edge;
        if (scanning) {
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
        scanning = edge;
        if (scanning) {
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
        scanning = edge;
        if (scanning) {
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
        scanning = edge;
        if (scanning) {
          layer.tile_lookup[{x, y}].push_back(layer.edges.size());
        }
      }
    }
  }
}

glm::vec2 Collision::project_xz(const Box& box, const glm::vec3& position,
                                const glm::vec2& projection_xz) const {
  static const std::size_t kCorners = 4;
  static const std::uint32_t kMaxIterations = 8;

  auto layer_it = layers_.find(coords(position.y));
  if (layer_it == layers_.end()) {
    return projection_xz;
  }
  const auto& layer = layer_it->second;

  glm::vec2 corners[kCorners] = {glm::vec2{-box.radius, 0.f}, glm::vec2{box.radius, 0.f},
                                 glm::vec2{0.f, -box.radius}, glm::vec2{0.f, box.radius}};

  // Find bounding box of projection volume.
  glm::vec2 xz = {position.x, position.z};
  glm::ivec2 min;
  glm::ivec2 max;
  bool first = true;
  for (std::size_t i = 0; i < kCorners; ++i) {
    if (first) {
      min = max = glm::ivec2{corners[i]};
    }
    min = glm::min(min, coords(corners[i] + xz - kTolerance));
    max = glm::max(max, coords(corners[i] + xz + kTolerance));
    min = glm::min(min, coords(corners[i] + xz + projection_xz - kTolerance));
    max = glm::max(max, coords(corners[i] + xz + projection_xz + kTolerance));
    first = false;
  }

  // Look up all edges we might intersect.
  std::unordered_set<std::size_t> edges;
  for (std::int32_t y = min.y; y <= max.y; ++y) {
    for (std::int32_t x = min.x; x <= max.x; ++x) {
      auto it = layer.tile_lookup.find({x, y});
      if (it != layer.tile_lookup.end()) {
        edges.insert(it->second.begin(), it->second.end());
      }
    }
  }

  // Test projection.
  float total_remaining = 1.f;
  std::uint32_t iteration = 0;
  glm::vec2 result_vector;
  glm::vec2 current_projection = projection_xz;
  while (true) {
    float collision_point = 1.f;
    const Edge* collision_edge = nullptr;
    for (const auto& edge_index : edges) {
      const auto& edge = layer.edges[edge_index];
      if (!can_collide(edge, current_projection)) {
        continue;
      }
      for (std::size_t i = 0; i < kCorners; ++i) {
        auto t = project(edge, corners[i] + xz + result_vector, current_projection);
        if (t < collision_point) {
          collision_point = t;
          collision_edge = &edge;
        }
      }
    }

    result_vector += collision_point * current_projection;
    if ((1.f - collision_point) * glm::dot(current_projection, current_projection) < kToleranceSq) {
      break;
    }
    // Project remaining portion of projection onto edge and continue.
    total_remaining *= (1.f - collision_point);
    auto remaining = total_remaining * projection_xz;
    auto edge = glm::normalize(collision_edge->b - collision_edge->a);
    auto new_projection = glm::dot(edge, remaining) * edge;
    if (++iteration == kMaxIterations || new_projection == glm::vec2{} ||
        new_projection == current_projection) {
      break;
    }
    current_projection = new_projection;
  }
  return result_vector;
}

}  // ::core
}  // ::gloam
