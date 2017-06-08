#include "workers/client/src/connect_mode.h"
#include "common/src/common/definitions.h"
#include "workers/client/src/input.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/world/player_controller.h"
#include <glm/vec4.hpp>
#include <schema/master.h>
#include <schema/player.h>
#include <iostream>

namespace gloam {

ConnectMode::ConnectMode(ModeState& mode_state, const std::string& disconnect_reason)
: mode_state_{mode_state}, connected_{false}, disconnect_reason_{disconnect_reason} {}

ConnectMode::ConnectMode(ModeState& mode_state, worker::Connection&& connection)
: mode_state_{mode_state}, connection_{new worker::Connection{std::move(connection)}} {
  dispatcher_.OnDisconnect([this](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected_ = false;
    disconnect_reason_ = op.Reason;
  });

  dispatcher_.OnLogMessage([this](const worker::LogMessageOp& op) {
    switch (op.Level) {
    case worker::LogLevel::kDebug:
      std::cout << "[debug] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kInfo:
      std::cout << "[info] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kWarn:
      std::cerr << "[warning] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kError:
      std::cerr << "[error] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kFatal:
      std::cerr << "[fatal] " << op.Message << std::endl;
      connected_ = false;
    default:
      break;
    }
  });

  dispatcher_.OnMetrics([this](const worker::MetricsOp& op) {
    auto metrics = op.Metrics;
    connection_->SendMetrics(metrics);
  });

  dispatcher_.OnCommandResponse<schema::Master::Commands::ClientHeartbeat>(
      [&](const worker::CommandResponseOp<schema::Master::Commands::ClientHeartbeat>& op) {
        if (op.StatusCode != worker::StatusCode::kSuccess) {
          std::cerr << "[warning] Heartbeat failed with code "
                    << static_cast<std::uint32_t>(op.StatusCode) << ": " << op.Message << std::endl;
        }
      });

  // Wait until we're getting updates from the ambient worker.
  dispatcher_.OnAuthorityChange<schema::PlayerClient>([&](const worker::AuthorityChangeOp& op) {
    have_stream_ &= op.HasAuthority;
    if (op.HasAuthority) {
      player_entity_id_ = op.EntityId;
    } else {
      player_entity_id_.clear();
    }
  });
  dispatcher_.OnComponentUpdate<schema::PlayerServer>(
      [&](const worker::ComponentUpdateOp<schema::PlayerServer>&) { have_stream_ = true; });

  if (!connection_->IsConnected()) {
    return;
  }
  connection_->SendLogMessage(worker::LogLevel::kInfo, "client", "Connected.");
  world_.reset(new world::PlayerController{*connection_, dispatcher_, mode_state_});
}

void ConnectMode::tick(const Input& input) {
  if (!connected_ && input.pressed(Button::kAnyKey)) {
    disconnect_ack_ = true;
  }
  if (connected_) {
    dispatcher_.Process(connection_->GetOpList(/* millis */ 0));

    // Send heartbeat periodically to master.
    if (frame_++ % 256 == 0) {
      connection_->SendCommandRequest<schema::Master::Commands::ClientHeartbeat>(
          /* master entity */ common::kMasterSeedEntityId, {player_entity_id_}, {});
    }
    if (have_stream_ && frame_ % 64 == 0 && !logged_in_) {
      logged_in_ = true;
      enter_frame_ = frame_;
    }
    if (have_stream_) {
      world_->tick(input);
    }
    if (logged_in_ && !have_stream_) {
      connected_ = false;
      disconnect_reason_ = "Lost player entity.";
    }
  } else if (disconnect_reason_.empty() || disconnect_ack_) {
    mode_state_.exit_to_title = true;
  }
}

void ConnectMode::sync() {
  static const std::string kClientLoadMetric = "client_load";

  if (connected_ && have_stream_) {
    world_->sync();
    // Send metrics for the inspector.
    worker::Metrics client_metrics;
    client_metrics.GaugeMetrics[kClientLoadMetric] = mode_state_.client_load;
    connection_->SendMetrics(client_metrics);
  }
}

void ConnectMode::render(const Renderer& renderer) const {
  auto dimensions = renderer.framebuffer_dimensions();
  if (!connected_) {
    auto text_width = renderer.text_width(disconnect_reason_);
    renderer.draw_text(disconnect_reason_, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, 1.f});
    return;
  } else if (!logged_in_) {
    auto fade = (frame_ % 64) / 64.f;
    std::string text = "ENTERING WORLD...";
    auto text_width = renderer.text_width(text);
    renderer.draw_text(text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, fade});
  } else {
    world_->render(renderer, mode_state_.frame);

    auto fade =
        std::max(0.f, std::min(1.f, 1.f - static_cast<float>(frame_ - enter_frame_) / 128.f));
    if (fade > 0.f) {
      renderer.draw_quad_colour({0.f, 0.f, 0.f, fade});
    }
  }
}

}  // ::gloam
