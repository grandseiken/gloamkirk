#ifndef GLOAM_COMMON_SRC_COMMON_HASHES_H
#define GLOAM_COMMON_SRC_COMMON_HASHES_H
#include "common/src/common/definitions.h"
#include <glm/vec2.hpp>
#include <improbable/standard_library.h>

namespace std {

template <typename T, glm::precision P>
struct hash<glm::tvec2<T, P>> {
  std::size_t operator()(const glm::tvec2<T, P>& v) const {
    std::size_t seed = 0;
    gloam::common::hash_combine(seed, v.x);
    gloam::common::hash_combine(seed, v.y);
    return seed;
  }
};

template <typename T, glm::precision P>
struct hash<glm::tvec3<T, P>> {
  std::size_t operator()(const glm::tvec3<T, P>& v) const {
    std::size_t seed = 0;
    gloam::common::hash_combine(seed, v.x);
    gloam::common::hash_combine(seed, v.y);
    gloam::common::hash_combine(seed, v.z);
    return seed;
  }
};

template <typename T, glm::precision P>
struct hash<glm::tvec4<T, P>> {
  std::size_t operator()(const glm::tvec4<T, P>& v) const {
    std::size_t seed = 0;
    gloam::common::hash_combine(seed, v.x);
    gloam::common::hash_combine(seed, v.y);
    gloam::common::hash_combine(seed, v.z);
    gloam::common::hash_combine(seed, v.w);
    return seed;
  }
};

template <>
struct hash<improbable::WorkerAttributeSet> {
  std::size_t operator()(const improbable::WorkerAttributeSet& attribute_set) const {
    std::size_t seed = 0;
    for (const auto& attribute : attribute_set.attribute()) {
      if (attribute.name()) {
        gloam::common::hash_combine(seed, *attribute.name());
      }
    }
    return seed;
  }
};

}  // ::std

#endif