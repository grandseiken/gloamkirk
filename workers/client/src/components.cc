#include "workers/client/src/components.h"
#include <improbable/standard_library.h>
#include <improbable/worker.h>
#include <schema/chunk.h>
#include <schema/common.h>
#include <schema/master.h>
#include <schema/player.h>

namespace gloam {
const worker::ComponentRegistry& ClientComponents() {
  using Components =
      worker::Components<schema::Chunk, schema::Master, schema::PlayerClient, schema::PlayerServer,
                         schema::InterpolatedPosition, improbable::Position>;
  static Components components;
  return components;
}
}  // ::gloam
