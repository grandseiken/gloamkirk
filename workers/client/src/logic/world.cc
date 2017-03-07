#include "workers/client/src/logic/world.h"
#include <improbable/worker.h>

namespace gloam {
namespace logic {

World::World(worker::Connection& connection, worker::Dispatcher& dispatcher)
: connection_{connection}, dispatcher_{dispatcher} {}

}  // ::logic
}  // ::gloam
