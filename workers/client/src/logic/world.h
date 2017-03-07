#ifndef GLOAM_WORKERS_CLIENT_SRC_LOGIC_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_LOGIC_WORLD_H

namespace worker {
class Connection;
class Dispatcher;
}  // ::worker

namespace gloam {
namespace logic {

class World {
public:
  World(worker::Connection& connection_, worker::Dispatcher& dispatcher_);

private:
  worker::Connection& connection_;
  worker::Dispatcher& dispatcher_;
};

}  // ::logic
}  // ::gloam

#endif
