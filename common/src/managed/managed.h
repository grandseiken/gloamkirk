#ifndef GLOAM_COMMON_SRC_MANAGED_MANAGED_H
#define GLOAM_COMMON_SRC_MANAGED_MANAGED_H
#include <string>

namespace worker {
class Connection;
class Dispatcher;
}  // ::worker

namespace gloam {
namespace managed {

class ManagedLogger {
public:
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

class ManagedWorker {
public:
  virtual void init(ManagedConnection& connection) = 0;
  virtual void update(ManagedConnection& connection) = 0;
};

// Parse standard command-line and connect.
int connect(const std::string& worker_type, ManagedWorker& worker, int argc, char** argv);

}  // ::managed
}  // ::gloam

#endif
