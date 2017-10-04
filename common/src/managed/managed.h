#ifndef GLOAM_COMMON_SRC_MANAGED_MANAGED_H
#define GLOAM_COMMON_SRC_MANAGED_MANAGED_H
#include <string>
#include <vector>

namespace worker {
class Connection;
class Dispatcher;
class ComponentRegistry;
}  // ::worker

namespace gloam {
namespace managed {

class ManagedLogger {
public:
  virtual ~ManagedLogger() = default;
  virtual void info(const std::string& message) const = 0;
  virtual void warn(const std::string& message) const = 0;
  virtual void error(const std::string& message) const = 0;
  virtual void fatal(const std::string& message) const = 0;
};

struct ManagedConnection {
  ManagedLogger& logger;
  worker::Connection& connection;
  worker::Dispatcher& dispatcher;
};

class WorkerLogic {
public:
  virtual ~WorkerLogic() = default;
  virtual void init(ManagedConnection& connection) = 0;
  // Called every tick (60 times per second).
  virtual void tick() = 0;
  // Called every sync (20 times per second).
  virtual void sync() = 0;
};

// Parse standard command-line and connect.
int connect(const worker::ComponentRegistry& registry, const std::string& worker_type,
            const std::vector<WorkerLogic*>& logic, bool enable_protocol_logging, int argc,
            char** argv);

}  // ::managed
}  // ::gloam

#endif
