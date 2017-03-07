#ifndef GLOAM_COMMON_SRC_COMMON_DEFINITIONS_H
#define GLOAM_COMMON_SRC_COMMON_DEFINITIONS_H
#include <improbable/standard_library.h>
#include <cstdint>
#include <string>

namespace gloam {
namespace common {
namespace {

const std::string kClientAtom = "client";
const std::string kMasterAtom = "master";
const improbable::WorkerAttribute kClientAttribute = {{kClientAtom}};
const improbable::WorkerAttribute kMasterAttribute = {{kMasterAtom}};
const improbable::WorkerRequirementSet kMasterOnlySet = {{{{kMasterAttribute}}}};
const improbable::WorkerRequirementSet kAllWorkersSet = {{{{kClientAttribute}}}};

const std::string kMasterSeedPrefab = "MasterSeed";
const std::string kChunkPrefab = "Chunk";
const std::string kPlayerPrefab = "Player";

template <typename T>
void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}  // anonymous
}  // ::common
}  // ::gloam

#endif