#ifndef GLOAM_WORKERS_MASTER_SRC_COMMON_DATA_H
#define GLOAM_WORKERS_MASTER_SRC_COMMON_DATA_H
#include "common/src/managed/managed.h"
#include <improbable/standard_library.h>
#include <schema/master.h>
#include <string>

namespace {
template <typename T>
void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
}  // anonymous

namespace gloam {
namespace master {
namespace {
const std::string kClientAttribute = "client";
const std::string kMasterAttribute = "master";
const improbable::WorkerRequirementSet kAllWorkers = {{{{{kClientAttribute}}}}};
}  // anonymous

class MasterData : public gloam::managed::WorkerLogic {
public:
  MasterData();
  const schema::MasterData& data();

  void init(managed::ManagedConnection& c) override;
  void update() override;

private:
  schema::MasterData data_;
};

}  // ::master
}  // ::gloam

#endif
