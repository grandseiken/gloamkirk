#ifndef GLOAM_WORKERS_CLIENT_SRC_CONNECT_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_CONNECT_MODE_H
#include "workers/client/src/glo.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/renderer.h"
#include <improbable/worker.h>

namespace gloam {

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
};

}  // ::gloam

#endif
