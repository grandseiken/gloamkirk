#include "common/src/common/definitions.h"
#include <improbable/standard_library.h>
#include <improbable/worker.h>
#include <schema/master.h>
#include <iostream>
#include <string>

namespace {
worker::Option<std::string> generate(worker::SnapshotOutputStream& stream) {
  improbable::EntityAclData entity_acl{
      gloam::common::kMasterOnlySet,
      {{gloam::schema::Master::ComponentId, gloam::common::kMasterOnlySet}}};

  worker::Entity master_seed_entity;
  master_seed_entity.Add<gloam::schema::Master>({false, {}});
  master_seed_entity.Add<improbable::EntityAcl>(entity_acl);
  master_seed_entity.Add<improbable::Metadata>({gloam::common::kMasterSeedEntityType});
  master_seed_entity.Add<improbable::Persistence>({});
  master_seed_entity.Add<improbable::Position>({{0., 0., 0.}});

  return stream.WriteEntity(gloam::common::kMasterSeedEntityId, master_seed_entity);
}
}  // anonymous

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "[error] Usage: " << argv[0] << " output/path" << std::endl;
    return 1;
  }

  using Components =
      worker::Components<gloam::schema::Master, improbable::EntityAcl, improbable::Metadata,
                         improbable::Persistence, improbable::Position>;
  {
    worker::SnapshotOutputStream stream{Components{}, argv[1]};
    auto error = generate(stream);
    if (error) {
      std::cerr << "[error] " << *error << std::endl;
      return 1;
    }
    std::cout << "[info] Wrote " << argv[1] << "." << std::endl;
  }
  return 0;
}
