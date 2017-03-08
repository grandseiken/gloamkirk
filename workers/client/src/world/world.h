#ifndef GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_H
#include "common/src/common/hashes.h"
#include "workers/client/src/world/world_renderer.h"
#include <improbable/worker.h>
#include <schema/chunk.h>
#include <unordered_map>

namespace worker {
class Connection;
class Dispatcher;
}  // ::worker

namespace gloam {
class Renderer;

namespace world {

class World {
public:
  World(worker::Connection& connection_, worker::Dispatcher& dispatcher_);
  void set_player_id(worker::EntityId player_id);
  void render(const Renderer& renderer) const;

private:
  void update_chunk(const schema::ChunkData& data);
  void clear_chunk(const schema::ChunkData& data);

  worker::Connection& connection_;
  worker::Dispatcher& dispatcher_;
  worker::EntityId player_id_;

  std::unordered_map<worker::EntityId, improbable::math::Coordinates> entity_positions_;
  std::unordered_map<worker::EntityId, schema::ChunkData> chunk_map_;
  std::unordered_map<glm::ivec2, schema::Tile> tile_map_;
  WorldRenderer world_renderer_;
};

}  // ::world
}  // ::gloam

#endif
