#include "workers/master/src/world_spawner.h"
#include <improbable/worker.h>
#include <schema/chunk.h>
#include <schema/common.h>
#include <unordered_set>

namespace gloam {
namespace master {
namespace {
const std::string kChunkSizeFlag = "chunk_size";
const std::int32_t kChunkSizeDefault = 8;
const std::int32_t kWorldSize = 1;

std::string coords_string(const glm::ivec2& coords) {
  return "(" + std::to_string(coords.x) + ", " + std::to_string(coords.y) + ")";
}

}  // anonymous

WorldSpawner::WorldSpawner(const schema::MasterData& master_data) : master_data_{master_data} {}

void WorldSpawner::init(managed::ManagedConnection& c) {
  c_.reset(new managed::ManagedConnection{c});

  auto flag_option = c.connection.GetWorkerFlag(kChunkSizeFlag);
  if (flag_option) {
    chunk_size_ = static_cast<double>(std::stoi(*flag_option));
  } else {
    chunk_size_ = kChunkSizeDefault;
  }

  c.dispatcher.OnAuthorityChange<schema::Master>([&](const worker::AuthorityChangeOp& op) {
    if (op.HasAuthority && !master_data_.world_spawned()) {
      // Plan for chunk spawning.
      for (std::int32_t x = -kWorldSize; x <= kWorldSize; ++x) {
        for (std::int32_t y = -kWorldSize; y <= kWorldSize; ++y) {
          chunks_[{x, y}];
        }
      }
    }
  });

  c.dispatcher.OnReserveEntityIdResponse([&](const worker::ReserveEntityIdResponseOp& op) {
    glm::ivec2 coords;
    bool found = false;
    for (const auto& pair : chunks_) {
      if (pair.second.reserve_request_id == op.RequestId) {
        coords = pair.first;
        found = true;
      }
    }
    if (!found) {
      return;
    }

    auto& info = chunks_[coords];
    info.reserve_request_id.Id = 0;
    if (op.StatusCode != worker::StatusCode::kSuccess) {
      c.logger.warn("Reserve entity ID failed for chunk " + coords_string(coords) + ": " +
                    op.Message);
      return;
    }
    info.entity_id = op.EntityId.value_or(-1);
  });

  c.dispatcher.OnCreateEntityResponse([&](const worker::CreateEntityResponseOp& op) {
    glm::ivec2 coords;
    bool found = false;
    for (const auto& pair : chunks_) {
      if (pair.second.create_request_id == op.RequestId) {
        coords = pair.first;
        found = true;
      }
    }
    if (!found) {
      return;
    }

    auto& info = chunks_[coords];
    info.create_request_id.Id = 0;
    if (op.StatusCode != worker::StatusCode::kSuccess) {
      c.logger.warn("Create entity failed for chunk " + coords_string(coords) + ": " + op.Message);
      return;
    }
    info.entity_created = true;
  });
}

void WorldSpawner::update() {
  std::unordered_set<glm::ivec2> chunks_stored;
  for (const auto& info : master_data_.chunks()) {
    glm::ivec2 coord = {info.x(), info.y()};
    chunks_[{info.x(), info.y()}].entity_created = true;
    chunks_[{info.x(), info.y()}].entity_id = info.entity_id();
    chunks_stored.insert(coord);
  }

  auto create_chunk_entity = [&](const glm::ivec2& coords, worker::EntityId entity_id) {
    improbable::EntityAclData entity_acl{{kAllWorkers}, {}};

    worker::Entity entity;
    entity.Add<schema::Chunk>({});
    entity.Add<schema::Position>({{coords.x * chunk_size_, 0., coords.y * chunk_size_}});
    entity.Add<improbable::EntityAcl>(entity_acl);
    return c_->connection.SendCreateEntityRequest(entity, {"Chunk"}, {entity_id}, {});
  };

  worker::List<schema::ChunkInfo> chunks_spawned;
  for (auto& pair : chunks_) {
    auto& info = pair.second;

    if (info.entity_id < 0 && !info.reserve_request_id.Id) {
      info.reserve_request_id = c_->connection.SendReserveEntityIdRequest({});
    }
    if (info.entity_id >= 0 && !info.entity_created && !info.create_request_id.Id) {
      info.create_request_id = create_chunk_entity(pair.first, info.entity_id);
    }
    if (info.entity_id >= 0 && info.entity_created) {
      chunks_spawned.emplace_back(info.entity_id, pair.first.x, pair.first.y);
    }
  }

  if (chunks_spawned.size() != chunks_stored.size()) {
    schema::Master::Update update;
    update.set_world_spawned(chunks_spawned.size() == chunks_.size());
    update.set_chunks(chunks_spawned);
    c_->connection.SendComponentUpdate<schema::Master>(0, update);
  }
}

}  // ::master
}  // ::gloam
