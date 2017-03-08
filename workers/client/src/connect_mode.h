#ifndef GLOAM_WORKERS_CLIENT_SRC_CONNECT_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_CONNECT_MODE_H
#include "workers/client/src/mode.h"
#include <improbable/worker.h>
#include <cstdint>
#include <memory>
#include <string>

namespace gloam {
class Renderer;
namespace logic {
class World;
}  // ::logic

class ConnectMode : public Mode {
public:
  ConnectMode(const std::string& disconnect_reason);
  ConnectMode(worker::Connection&& connection);
  ModeResult event(const sf::Event& event) override;
  ModeResult update() override;
  void render(const Renderer& renderer) const override;

private:
  bool connected_ = true;
  bool disconnect_ack_ = false;
  std::string disconnect_reason_;

  std::unique_ptr<worker::Connection> connection_;
  worker::Dispatcher dispatcher_;

  bool logged_in_ = false;
  std::unique_ptr<logic::World> world_;
  worker::EntityId player_id_ = -1;

  std::uint64_t frame_ = 0;
  std::uint64_t enter_frame_ = 0;
};

}  // ::gloam

#endif
