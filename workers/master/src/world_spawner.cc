#include "workers/master/src/world_spawner.h"
#include "common/src/common/definitions.h"
#include <improbable/worker.h>
#include <schema/chunk.h>
#include <schema/common.h>
#include <unordered_set>

namespace gloam {
namespace master {
namespace {
const std::string kChunkSizeFlag = "chunk_size";
const std::int32_t kChunkSizeDefault = 8;
const std::int32_t kWorldSize = 4;

std::string coords_string(const glm::ivec2& coords) {
  return "(" + std::to_string(coords.x) + ", " + std::to_string(coords.y) + ")";
}

}  // anonymous

WorldSpawner::WorldSpawner(const schema::MasterData& master_data) : master_data_{master_data} {}

void WorldSpawner::init(managed::ManagedConnection& c) {
  c_.reset(new managed::ManagedConnection{c});

  auto flag_option = c.connection.GetWorkerFlag(kChunkSizeFlag);
  if (flag_option) {
    chunk_size_ = static_cast<std::int32_t>(std::stoi(*flag_option));
  } else {
    chunk_size_ = kChunkSizeDefault;
  }

  c.dispatcher.OnAuthorityChange<schema::Master>([&](const worker::AuthorityChangeOp& op) {
    if (op.HasAuthority && !master_data_.world_spawned()) {
      // Plan for chunk spawning.
      for (std::int32_t x = 0; x < kWorldSize; ++x) {
        for (std::int32_t y = 0; y < kWorldSize; ++y) {
          chunks_[{x, y}];
          chunks_[{-x - 1, y}];
          chunks_[{x, -y - 1}];
          chunks_[{-x - 1, -y - 1}];
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

void WorldSpawner::sync() {
  std::unordered_set<glm::ivec2> chunks_stored;
  for (const auto& info : master_data_.chunks()) {
    glm::ivec2 coord = {info.x(), info.y()};
    chunks_[{info.x(), info.y()}].entity_created = true;
    chunks_[{info.x(), info.y()}].entity_id = info.entity_id();
    chunks_stored.insert(coord);
  }

  auto create_chunk_entity = [&](const glm::ivec2& coords, worker::EntityId entity_id) {
    improbable::EntityAclData entity_acl{common::kAllWorkersSet, {}};

    schema::ChunkData chunk_data{chunk_size_, coords.x, coords.y, {}};
    for (std::int32_t y = 0; y < chunk_size_; ++y) {
      for (std::int32_t x = 0; x < chunk_size_; ++x) {
        auto cs2 = chunk_size_ / 2;
        auto ramp = (3 + x == cs2 || 2 + x == cs2) && (y == cs2 || 1 + y == cs2)
            ? schema::Tile::Ramp::RIGHT
            : ((x == cs2 || 1 + x == cs2) && y - 1 == cs2) ||
                    (1 + x == chunk_size_ && 1 + y == chunk_size_)
                ? schema::Tile::Ramp::RIGHT
                : (x - 2 == cs2 || x - 1 == cs2) && (y == cs2 || 1 + y == cs2)
                    ? schema::Tile::Ramp::LEFT
                    : (x == cs2 || 1 + x == cs2) && 2 + y == cs2 ? schema::Tile::Ramp::LEFT
                                                                 : schema::Tile::Ramp::NONE;
        chunk_data.tiles().emplace_back(
            schema::Tile::Terrain::GRASS,
            (!x && !y) || ((x == cs2 || 1 + x == cs2) && (y == cs2 || 1 + y == cs2))
                ? 2
                : (x - 1 == cs2 || 2 + x == cs2) && (y == cs2 || 1 + y == cs2) ? 1 : 0,
            ramp);
      }
    }

    auto chunk_size = static_cast<double>(chunk_size_);
    worker::Entity entity;
    entity.Add<schema::CanonicalPosition>(
        {{chunk_size / 2 + coords.x * chunk_size, 0., chunk_size / 2 + coords.y * chunk_size}});
    entity.Add<schema::Chunk>(chunk_data);
    entity.Add<improbable::EntityAcl>(entity_acl);
    return c_->connection.SendCreateEntityRequest(entity, {common::kChunkPrefab}, {entity_id}, {});
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
