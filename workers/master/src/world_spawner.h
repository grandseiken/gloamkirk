#ifndef GLOAM_WORKERS_MASTER_SRC_WORLD_SPAWNER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLD_SPAWNER_H
#include "common/src/common/definitions.h"
#include "common/src/managed/managed.h"
#include <glm/vec2.hpp>
#include <improbable/worker.h>
#include <schema/master.h>
#include <unordered_map>

namespace std {
template <>
struct hash<glm::ivec2> {
  std::size_t operator()(const glm::ivec2& v) const {
    std::size_t seed = 0;
    gloam::common::hash_combine(seed, v.x);
    gloam::common::hash_combine(seed, v.y);
    return seed;
  }
};
}

namespace gloam {
namespace master {

class WorldSpawner : public gloam::managed::WorkerLogic {
public:
  WorldSpawner(const schema::MasterData& master_data);
  void init(managed::ManagedConnection& c) override;
  void update() override;

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
  double chunk_size_;
  std::unique_ptr<managed::ManagedConnection> c_;
  std::unordered_map<glm::ivec2, ChunkInfo> chunks_;
};

}  // ::master
}  // ::gloam

#endif
