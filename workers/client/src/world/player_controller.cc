#include "workers/client/src/world/player_controller.h"
#include "common/src/common/conversions.h"
#include "common/src/common/timing.h"
#include "workers/client/src/input.h"
#include <glm/glm.hpp>
#include <schema/common.h>
#include <schema/player.h>

namespace gloam {
namespace world {

PlayerController::PlayerController(worker::Connection& connection, worker::Dispatcher& dispatcher,
                                   const ModeState& mode_state)
: connection_{connection}, dispatcher_{dispatcher}, player_id_{-1}, world_renderer_{mode_state} {
  dispatcher_.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection_.SendComponentInterest(op.EntityId,
                                      {
                                          // TODO: replace with interpolation component.
                                          {schema::CanonicalPosition::ComponentId, {true}},
                                          // TODO: replace with common component.
                                          {schema::PlayerClient::ComponentId, {true}},
                                      });
  });

  dispatcher_.OnAuthorityChange<schema::PlayerClient>([&](const worker::AuthorityChangeOp& op) {
    connection.SendComponentInterest(op.EntityId,
                                     {
                                         {schema::PlayerServer::ComponentId, {op.HasAuthority}},
                                     });
  });

  dispatcher_.OnAddComponent<schema::CanonicalPosition>(
      [&](const worker::AddComponentOp<schema::CanonicalPosition>& op) {
        entity_positions_.erase(op.EntityId);
        entity_positions_.insert(std::make_pair(op.EntityId, common::coords(op.Data.coords())));
      });

  dispatcher_.OnRemoveComponent<schema::CanonicalPosition>(
      [&](const worker::RemoveComponentOp& op) { entity_positions_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::CanonicalPosition>([&](
      const worker::ComponentUpdateOp<schema::CanonicalPosition>& op) {
    if (op.EntityId != player_id_ && op.Update.coords()) {
      entity_positions_.erase(op.EntityId);
      entity_positions_.insert(std::make_pair(op.EntityId, common::coords(*op.Update.coords())));
    }
  });

  dispatcher_.OnAddComponent<schema::PlayerClient>(
      [&](const worker::AddComponentOp<schema::PlayerClient>& op) {
        player_entities_.insert(op.EntityId);
      });

  dispatcher_.OnRemoveComponent<schema::PlayerClient>(
      [&](const worker::RemoveComponentOp& op) { player_entities_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::PlayerServer>(
      [&](const worker::ComponentUpdateOp<schema::PlayerServer>& op) {
        if (op.EntityId == player_id_ && !op.Update.sync_state().empty()) {
          const auto& sync_state = op.Update.sync_state().front();
          reconcile(sync_state.sync_tick(), {sync_state.x(), sync_state.y(), sync_state.z()});
        }
      });

  tile_map_.register_callbacks(connection_, dispatcher_);
}

void PlayerController::set_player_id(worker::EntityId player_id) {
  player_id_ = player_id;
  canonical_position_ = entity_positions_[player_id_];
}

void PlayerController::tick(const Input& input) {
  static const float kSnapMaxDistance = 1.f / 64;
  static const float kInterpolateMaxDistance = 1.f;
  static const float kMovingInterpolateMinDistance = 1.f / 8;

  if (tile_map_.has_changed()) {
    collision_.update(tile_map_);
  }

  glm::vec2 direction;
  if (input.held(Button::kLeft)) {
    direction += glm::vec2{-1.f, -1.f};
  }
  if (input.held(Button::kRight)) {
    direction += glm::vec2{1.f, 1.f};
  }
  if (input.held(Button::kDown)) {
    direction += glm::vec2{1.f, -1.f};
  }
  if (input.held(Button::kUp)) {
    direction += glm::vec2{-1.f, 1.f};
  }
  bool is_moving = direction != glm::vec2{};

  auto interpolate = [&](glm::vec3& from, const glm::vec3& to) {
    auto error = glm::length(to - from);
    if (!error || error > kInterpolateMaxDistance || (!is_moving && error < kSnapMaxDistance)) {
      from = to;
    } else {
      auto correction = 0.f;
      if (!is_moving || error > kSnapMaxDistance) {
        correction = kSnapMaxDistance;
      }
      if (!is_moving || error > kMovingInterpolateMinDistance) {
        correction = std::max(correction, error / 16.f);
      }
      from += correction * (to - from) / error;
    }
  };

  // Interpolate to canonical server position. We also interpolate the canonical position towards
  // our position to smooth out server hiccups.
  auto& position = entity_positions_[player_id_];
  interpolate(position, canonical_position_);
  interpolate(canonical_position_, position);

  if (is_moving) {
    direction = glm::normalize(direction);
    core::Box box{1.f / 8};

    auto speed_per_tick = common::kPlayerSpeed / common::kTicksPerSync;
    auto projection_xz = collision_.project_xz(box, position, speed_per_tick * direction);
    position += glm::vec3{projection_xz.x, 0.f, projection_xz.y};
    canonical_position_ += glm::vec3{projection_xz.x, 0.f, projection_xz.y};
    player_tick_dv_ += projection_xz / common::kPlayerSpeed;
  }
}

void PlayerController::sync() {
  static const std::size_t kMaxHistorySize = 256;

  // Need to send input every frame even if unchanged to avoid stalls on hard authority handover.
  // Should only send the sync tick every so often, though.
  bool send_tick = !(sync_tick_ % 256);
  ++sync_tick_;

  schema::PlayerInput input{{}, player_tick_dv_.x, player_tick_dv_.y};
  if (send_tick) {
    input.set_sync_tick(sync_tick_);
  }
  connection_.SendComponentUpdate<schema::PlayerClient>(
      player_id_, schema::PlayerClient::Update{}.add_sync_input(input));

  input_history_.push_back({sync_tick_, player_tick_dv_});
  if (input_history_.size() > kMaxHistorySize) {
    input_history_.pop_front();
  }
  player_tick_dv_ = {};
}

void PlayerController::render(const Renderer& renderer, std::uint64_t frame) const {
  auto it = entity_positions_.find(player_id_);
  if (it == entity_positions_.end()) {
    return;
  }
  std::vector<Light> lights;
  std::vector<glm::vec3> positions;

  lights.push_back({it->second + glm::vec3{0.f, 1.f, 0.f}, 2.f, 2.f});
  for (worker::EntityId entity_id : player_entities_) {
    auto jt = entity_positions_.find(entity_id);
    positions.push_back(jt->second);
    if (entity_id != player_id_ && jt != entity_positions_.end()) {
      lights.push_back({jt->second + glm::vec3{0.f, 1.f, 0.f}, 2.f, 2.f});
    }
  }

  world_renderer_.render(renderer, frame, it->second, lights, positions, tile_map_.get());
}

void PlayerController::reconcile(std::uint32_t sync_tick, const glm::vec3& coordinates) {
  // Discard old information.
  bool unsynced = !input_history_.empty() && sync_tick < input_history_.front().sync_tick;
  if (unsynced) {
    input_history_.pop_front();
  }
  while (!input_history_.empty() && input_history_.front().sync_tick <= sync_tick) {
    input_history_.pop_front();
  }

  // Recompute canonical position. Unsynced workers that don't have our tick number yet shouldn't
  // apply the entire history, though!
  canonical_position_ = coordinates;
  if (unsynced) {
    return;
  }
  for (const auto& input : input_history_) {
    core::Box box{1.f / 8};
    auto projection_xz =
        collision_.project_xz(box, canonical_position_, common::kPlayerSpeed * input.xz_dv);
    canonical_position_ += glm::vec3{projection_xz.x, 0.f, projection_xz.y};
  }
}

}  // ::world
}  // ::gloam
