#ifndef GLOAM_WORKERS_MASTER_SRC_WORLD_SPAWNER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLD_SPAWNER_H
#include "common/src/common/hashes.h"
#include "common/src/managed/managed.h"
#include <glm/vec2.hpp>
#include <improbable/worker.h>
#include <schema/master.h>
#include <unordered_map>

namespace gloam {
namespace master {

class WorldSpawner : public gloam::managed::WorkerLogic {
public:
  WorldSpawner(const schema::MasterData& master_data);
  void init(managed::ManagedConnection& c) override;
  void tick() override {}
  void sync() override;

private:
  struct ChunkInfo {
    bool entity_created = false;
    // Entity ID for this chunk.
    worker::EntityId entity_id = -1;
    // Reserve request ID.
    worker::RequestId<worker::ReserveEntityIdRequest> reserve_request_id;
    // Create request ID.
    worker::RequestId<worker::CreateEntityRequest> create_request_id;
  };

  const schema::MasterData& master_data_;
  std::int32_t chunk_size_;
  std::unique_ptr<managed::ManagedConnection> c_;
  std::unordered_map<glm::ivec2, ChunkInfo> chunks_;
};

}  // ::master
}  // ::gloam

#endif
