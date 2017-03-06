#ifndef GLOAM_CLIENT_SRC_CONNECT_MODE_H
#define GLOAM_CLIENT_SRC_CONNECT_MODE_H
#include "workers/client/src/glo.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/renderer.h"
#include <improbable/worker.h>

namespace gloam {

class ConnectMode : public Mode {
public:
  ConnectMode(worker::Connection&& connection);
  ModeAction event(const sf::Event& event) override;
  std::unique_ptr<Mode> update() override;
  void render(const Renderer& renderer) const override;

private:
  bool connected_ = true;
  worker::Connection connection_;
  worker::Dispatcher dispatcher_;
};

}  // ::gloam

#endif
