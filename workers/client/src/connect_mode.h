#ifndef GLOAM_WORKERS_CLIENT_SRC_CONNECT_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_CONNECT_MODE_H
#include "workers/client/src/mode.h"
#include <improbable/worker.h>
#include <cstdint>
#include <memory>
#include <string>

namespace gloam {
class Renderer;
namespace world {
class PlayerController;
}  // ::world

class ConnectMode : public Mode {
public:
  ConnectMode(ModeState& mode_state, const std::string& disconnect_reason);
  ConnectMode(ModeState& mode_state, worker::Connection&& connection);
  void tick(const Input& input) override;
  void sync() override;
  void render(const Renderer& renderer) const override;

private:
  ModeState& mode_state_;
  bool connected_ = true;
  bool disconnect_ack_ = false;
  std::string disconnect_reason_;

  std::unique_ptr<worker::Connection> connection_;
  worker::Dispatcher dispatcher_;

  worker::Option<worker::EntityId> player_entity_id_;
  std::unique_ptr<world::PlayerController> world_;
  bool have_stream_ = false;
  bool logged_in_ = false;

  std::uint64_t frame_ = 0;
  std::uint64_t enter_frame_ = 0;
};

}  // ::gloam

#endif
