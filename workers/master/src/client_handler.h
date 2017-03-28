#ifndef GLOAM_WORKERS_MASTER_SRC_CLIENT_HANDLER_H
#define GLOAM_WORKERS_MASTER_SRC_CLIENT_HANDLER_H
#include "common/src/common/hashes.h"
#include "common/src/managed/managed.h"
#include <improbable/standard_library.h>
#include <improbable/worker.h>
#include <schema/master.h>
#include <unordered_map>

namespace gloam {
namespace master {

class ClientHandler : public gloam::managed::WorkerLogic {
public:
  using Client = improbable::WorkerAttributeSet;
  ClientHandler(const schema::MasterData& master_data);

  void init(managed::ManagedConnection& c) override;
  void tick() override {}
  void sync() override;

private:
  struct ClientInfo {
    bool entity_created = false;
    // Player entity ID for this client.
    worker::EntityId entity_id = -1;
    // Timestamp of least heartbeat.
    std::uint64_t timestamp_millis = 0;
    // Reserve request ID.
    worker::RequestId<worker::ReserveEntityIdRequest> reserve_request_id;
    // Create request ID.
    worker::RequestId<worker::CreateEntityRequest> create_request_id;
    // Delete request ID.
    worker::RequestId<worker::DeleteEntityRequest> delete_request_id;
  };

  void update_client(const Client& client);

  const schema::MasterData& master_data_;
  std::unique_ptr<managed::ManagedConnection> c_;
  std::unordered_map<Client, ClientInfo> clients_;
};

}  // ::master
}  // ::gloam

#endif
