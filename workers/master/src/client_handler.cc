#include "workers/master/src/client_handler.h"
#include "common/src/common/definitions.h"
#include <schema/master.h>
#include <schema/player.h>
#include <algorithm>
#include <chrono>
#include <unordered_map>

namespace gloam {
namespace master {
namespace {
using Client = ClientHandler::Client;

Client to_client(const worker::List<std::string>& caller_attributes) {
  Client result = {{}};
  for (const auto& attribute : caller_attributes) {
    result.attribute().emplace_back(attribute);
  }
  return result;
}

std::uint64_t timestamp_millis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string attribute_string(const Client& client) {
  std::string result;
  bool first = true;
  for (const auto& attribute : client.attribute()) {
    if (!attribute.name()) {
      continue;
    }
    if (!first) {
      result += ", ";
    }
    result += *attribute.name();
    first = false;
  }
  return "{" + result + "}";
}

improbable::WorkerRequirementSet client_acl(const Client& client) {
  return {{client}};
}

bool check_client(const Client& client) {
  return std::find(client.attribute().begin(), client.attribute().end(),
                   worker::Option<std::string>{common::kClientAtom}) != client.attribute().end();
}

}  // anonymous

ClientHandler::ClientHandler(const schema::MasterData& master_data) : master_data_{master_data} {}

void ClientHandler::init(managed::ManagedConnection& c) {
  c_.reset(new managed::ManagedConnection{c});
  using ClientHeartbeat = schema::Master::Commands::ClientHeartbeat;

  c.dispatcher.OnCommandRequest<ClientHeartbeat>(
      [&](const worker::CommandRequestOp<ClientHeartbeat>& op) {
        auto client = to_client(op.CallerAttributeSet);
        if (!check_client(client)) {
          c.logger.warn("Received heartbeat from non-client " + attribute_string(client));
          return;
        }

        update_client(client);
        c.connection.SendCommandResponse<ClientHeartbeat>(op.RequestId, {});
      });

  c.dispatcher.OnReserveEntityIdResponse([&](const worker::ReserveEntityIdResponseOp& op) {
    Client client = {{}};
    for (const auto& pair : clients_) {
      if (pair.second.reserve_request_id == op.RequestId) {
        client = pair.first;
      }
    }
    if (client.attribute().empty()) {
      return;
    }

    auto& info = clients_[client];
    info.reserve_request_id.Id = 0;
    if (op.StatusCode != worker::StatusCode::kSuccess) {
      c.logger.warn("Reserve entity ID failed for client " + attribute_string(client) + ": " +
                    op.Message);
      return;
    }
    info.entity_id = op.EntityId.value_or(-1);
  });

  c.dispatcher.OnCreateEntityResponse([&](const worker::CreateEntityResponseOp& op) {
    Client client = {{}};
    for (const auto& pair : clients_) {
      if (pair.second.create_request_id == op.RequestId) {
        client = pair.first;
      }
    }
    if (client.attribute().empty()) {
      return;
    }

    auto& info = clients_[client];
    info.create_request_id.Id = 0;
    if (op.StatusCode != worker::StatusCode::kSuccess) {
      c.logger.warn("Create entity failed for client " + attribute_string(client) + ": " +
                    op.Message);
      // Next heartbeat will retry.
      clients_.erase(client);
      return;
    }
    c.logger.info("Created entity " + std::to_string(info.entity_id) + " for " +
                  attribute_string(client));
    info.entity_created = true;
  });

  c.dispatcher.OnDeleteEntityResponse([&](const worker::DeleteEntityResponseOp& op) {
    Client client = {{}};
    for (const auto& pair : clients_) {
      if (pair.second.delete_request_id == op.RequestId) {
        client = pair.first;
      }
    }
    if (client.attribute().empty()) {
      return;
    }

    auto& info = clients_[client];
    info.delete_request_id.Id = 0;
    if (op.StatusCode != worker::StatusCode::kSuccess) {
      c.logger.warn("Delete entity failed for client " + attribute_string(client) + ": " +
                    op.Message);
      // Wait before retrying.
      clients_[client].timestamp_millis = timestamp_millis();
      return;
    }
    c.logger.info("Deleted entity " + std::to_string(info.entity_id) + " for " +
                  attribute_string(client));
    info.entity_id = -1;
    info.entity_created = false;
  });
}

void ClientHandler::sync() {
  if (!master_data_.world_spawned()) {
    return;
  }
  static const std::uint64_t kDeleteEntityTimeoutMillis = 1 << 15;

  auto create_player_entity = [&](const Client& client, worker::EntityId entity_id) {
    worker::Map<worker::ComponentId, improbable::WorkerRequirementSet> acl_map = {
        {schema::PlayerClient::ComponentId, client_acl(client)},
        {schema::PlayerServer::ComponentId, common::kAmbientOnlySet},
        {schema::CanonicalPosition::ComponentId, common::kAmbientOnlySet}};
    improbable::EntityAclData entity_acl{common::kAllWorkersSet, {acl_map}};

    worker::Entity entity;
    entity.Add<schema::PlayerClient>({});
    entity.Add<schema::PlayerServer>({});
    entity.Add<schema::CanonicalPosition>({{0., 0., 0.}});
    entity.Add<improbable::EntityAcl>(entity_acl);
    return c_->connection.SendCreateEntityRequest(entity, {common::kPlayerPrefab}, {entity_id}, {});
  };

  std::vector<Client> expired_clients;
  auto now = timestamp_millis();

  for (auto& pair : clients_) {
    auto& info = pair.second;
    bool expired = now - info.timestamp_millis > kDeleteEntityTimeoutMillis;

    // If not expired, try to create an entity.
    if (!expired && info.entity_id < 0 && !info.reserve_request_id.Id) {
      info.reserve_request_id = c_->connection.SendReserveEntityIdRequest({});
    }
    if (!expired && info.entity_id >= 0 && !info.entity_created && !info.create_request_id.Id) {
      info.create_request_id = create_player_entity(pair.first, info.entity_id);
    }
    // If expired, try to delete the entity.
    if (expired && info.entity_created && !info.delete_request_id.Id) {
      info.delete_request_id = c_->connection.SendDeleteEntityRequest(info.entity_id, {});
    }
    if (expired && !info.entity_created && !info.create_request_id.Id) {
      expired_clients.push_back(pair.first);
    }
  }
  for (const auto& client : expired_clients) {
    clients_.erase(client);
  }
}

void ClientHandler::update_client(const Client& client) {
  clients_[client].timestamp_millis = timestamp_millis();
}

}  // ::master
}  // ::gloam
