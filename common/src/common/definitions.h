#ifndef GLOAM_COMMON_SRC_COMMON_DEFINITIONS_H
#define GLOAM_COMMON_SRC_COMMON_DEFINITIONS_H
#include <improbable/standard_library.h>
#include <cstdint>
#include <string>

namespace gloam {
namespace common {
namespace {
const std::string kAmbientAttribute = "ambient";
const std::string kClientAttribute = "client";
const std::string kMasterAttribute = "master";
const improbable::WorkerRequirementSet kAmbientOnlySet = {{{{kAmbientAttribute}}}};
const improbable::WorkerRequirementSet kClientOnlySet = {{{{kClientAttribute}}}};
const improbable::WorkerRequirementSet kMasterOnlySet = {{{{kMasterAttribute}}}};
const improbable::WorkerRequirementSet kAllWorkersSet = {
    {{{kAmbientAttribute}}, {{kClientAttribute}}}};

const worker::EntityId kMasterSeedEntityId = 1;
const std::string kMasterSeedEntityType = "MasterSeed";
const std::string kChunkEntityType = "Chunk";
const std::string kPlayerEntityType = "Player";
}  // anonymous
}  // ::common
}  // ::gloam

#endif
