#include "workers/client/src/world/player_controller.h"
#include "common/src/common/conversions.h"
#include "common/src/common/math.h"
#include "common/src/common/timing.h"
#include "workers/client/src/input.h"
#include <glm/glm.hpp>
#include <schema/common.h>
#include <schema/player.h>
#include <algorithm>

namespace gloam {
namespace world {
namespace {
// Interpolation parameters for remote entities.
const std::size_t kInterpolationBufferSize = 2;
// Interpolation parameters for local player against authoritative server.
const float kSnapMaxDistance = 1.f / 64;
const float kInterpolateMaxDistance = 1.f;
const float kMovingInterpolateMinDistance = 1.f / 8;

template <typename T>
void interpolate(T& from, const T& to, bool is_moving) {
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
}
}  // anonymous namespace

PlayerController::PlayerController(worker::Connection& connection, worker::Dispatcher& dispatcher,
                                   const ModeState& mode_state)
: connection_{connection}
, dispatcher_{dispatcher}
, collision_{tile_map_}
, world_renderer_{mode_state} {
  dispatcher_.OnAddEntity([&](const worker::AddEntityOp& op) {
    connection_.SendComponentInterest(op.EntityId,
                                      {
                                          // Only for initial checkout.
                                          {improbable::Position::ComponentId, {true}},
                                          {schema::InterpolatedPosition::ComponentId, {true}},
                                          // TODO: replace with common component.
                                          {schema::PlayerClient::ComponentId, {true}},
                                      });
  });

  dispatcher.OnAddComponent<improbable::Position>([&](
      const worker::AddComponentOp<improbable::Position>& op) {
    if (op.EntityId != player_id_) {
      interpolation_[op.EntityId].positions.push_back(
          {op.Data.coords().x(), op.Data.coords().y(), op.Data.coords().z()});
    }
    connection.SendComponentInterest(op.EntityId, {{improbable::Position::ComponentId, {false}}});
  });

  dispatcher.OnRemoveEntity(
      [&](const worker::RemoveEntityOp& op) { interpolation_.erase(op.EntityId); });

  dispatcher_.OnAuthorityChange<schema::PlayerClient>([&](const worker::AuthorityChangeOp& op) {
    player_id_ = op.EntityId;
    bool has_authority = op.Authority == worker::Authority::kAuthoritative;
    have_player_position_ = false;
    interpolation_.erase(player_id_);
    connection.SendComponentInterest(
        op.EntityId, {
                         {schema::PlayerServer::ComponentId, {has_authority}},
                         {schema::InterpolatedPosition::ComponentId, {!has_authority}},
                     });
  });

  dispatcher_.OnAddComponent<schema::PlayerClient>(
      [&](const worker::AddComponentOp<schema::PlayerClient>& op) {
        player_entities_.insert(op.EntityId);
      });

  dispatcher_.OnRemoveComponent<schema::PlayerClient>(
      [&](const worker::RemoveComponentOp& op) { player_entities_.erase(op.EntityId); });

  dispatcher_.OnComponentUpdate<schema::PlayerServer>(
      [&](const worker::ComponentUpdateOp<schema::PlayerServer>& op) {
        if (!op.Update.sync_state().empty()) {
          const auto& sync_state = op.Update.sync_state().front();
          reconcile(sync_state.sync_tick(), {sync_state.x(), sync_state.y(), sync_state.z()});
        }
      });

  dispatcher_.OnComponentUpdate<schema::InterpolatedPosition>(
      [&](const worker::ComponentUpdateOp<schema::InterpolatedPosition>& op) {
        if (op.EntityId != player_id_) {
          auto& interpolation = interpolation_[op.EntityId];
          for (const auto& position : op.Update.position()) {
            interpolation.positions.push_back({position.x(), position.y(), position.z()});
          }
        }
      });

  tile_map_.register_callbacks(connection_, dispatcher_);
}

void PlayerController::tick(const Input& input) {
  collision_.update();
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

  // Interpolate to canonical server position. We also interpolate the canonical position towards
  // our position to smooth out server hiccups.
  auto local_xz = common::get_xz(local_position_);
  auto canonical_xz = common::get_xz(canonical_position_);
  interpolate(local_xz, canonical_xz, is_moving);
  interpolate(canonical_xz, local_xz, is_moving);
  local_position_ = common::from_xz(local_xz, local_position_.y);
  canonical_position_ = common::from_xz(canonical_xz, canonical_position_.y);
  // Also interpolate on the Y-axis, but do it separately.
  interpolate(local_position_.y, canonical_position_.y, false);
  interpolate(canonical_position_.y, local_position_.y, false);

  auto speed_per_tick = common::kPlayerSpeed / common::kTicksPerSync;
  auto gravity = common::kGravity / common::kTicksPerSync;

  core::Box box{1.f / 8};
  if (is_moving) {
    direction = glm::normalize(direction);
    auto projection_xz = collision_.project_xz(box, local_position_, speed_per_tick * direction);

    local_position_ += common::from_xz(projection_xz, 0.f);
    canonical_position_ += common::from_xz(projection_xz, 0.f);
    player_tick_dv_ += projection_xz / common::kPlayerSpeed;
  }
  local_position_.y -= gravity;
  local_position_.y = collision_.terrain_height(box, local_position_);
  canonical_position_.y -= gravity;
  canonical_position_.y = collision_.terrain_height(box, canonical_position_);

  for (auto& pair : interpolation_) {
    auto& interpolation = pair.second;
    while (interpolation.positions.size() > 2 * kInterpolationBufferSize) {
      interpolation.index = 0;
      interpolation.positions.pop_front();
    }
    if (interpolation.positions.size() > kInterpolationBufferSize &&
        ++interpolation.index >= common::kTicksPerSync) {
      interpolation.index = 0;
      interpolation.positions.pop_front();
    }
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
  // TODO: screen sometimes stops rendering for no reason.
  std::vector<Light> lights;
  std::vector<glm::vec3> positions;

  auto interpolated_position = [&](const Interpolation& interpolation) {
    auto base = interpolation.positions.front();
    auto next = interpolation.positions.size() > 1 ? interpolation.positions.begin()[1] : base;
    return base + (next - base) * (interpolation.index / static_cast<float>(common::kTicksPerSync));
  };

  positions.push_back(local_position_);
  lights.push_back({local_position_ + glm::vec3{0.f, 1.f, 0.f}, 2.f, 2.f});
  for (worker::EntityId entity_id : player_entities_) {
    auto it = interpolation_.find(entity_id);
    if (it != interpolation_.end() && !it->second.positions.empty()) {
      auto position = interpolated_position(it->second);
      positions.push_back(position);
      if (entity_id != player_id_) {
        lights.push_back({position + glm::vec3{0.f, 1.f, 0.f}, 2.f, 2.f});
      }
    }
  }

  world_renderer_.render(renderer, frame, local_position_, lights, positions, tile_map_.get());
}

void PlayerController::reconcile(std::uint32_t sync_tick, const glm::vec3& coordinates) {
  if (!have_player_position_) {
    local_position_ = coordinates;
    have_player_position_ = true;
  }

  // Discard old information.
  bool unsynced = !input_history_.empty() && sync_tick < input_history_.front().sync_tick;
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
    canonical_position_ += common::from_xz(projection_xz, 0.f);
  }
}

}  // ::world
}  // ::gloam
