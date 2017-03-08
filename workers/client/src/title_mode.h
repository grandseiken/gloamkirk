#ifndef GLOAM_WORKERS_CLIENT_SRC_TITLE_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_TITLE_MODE_H
#include "workers/client/src/glo.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/renderer.h"
#include <improbable/worker.h>
#include <cstdint>
#include <memory>

namespace gloam {

class TitleMode : public Mode {
public:
  TitleMode(bool first_run, bool fade_in, bool local,
            const worker::ConnectionParameters& connection_params,
            const worker::LocatorParameters& locator_params);
  ModeResult event(const sf::Event& event) override;
  ModeResult update() override;
  void render(const Renderer& renderer) const override;

private:
  enum MenuItem {
    kConnect,
    kToggleFullscreen,
    kExitApplication,
    kMenuItemCount,
  };

  TextureImage title_;
  glo::Program title_program_;

  std::uint64_t fade_in_ = 0;
  std::uint64_t connect_frame_ = 0;
  std::uint64_t finish_connect_frame_ = 0;
  std::int32_t menu_item_ = MenuItem::kConnect;

  bool connection_local_;
  bool connection_cancel_ = false;
  worker::ConnectionParameters connection_params_;
  worker::Locator locator_;
  // Workaround for broken future move-constructor.
  struct LocatorFutureWrapper {
    worker::Future<worker::DeploymentList> future;
  };
  struct ConnectionFutureWrapper {
    worker::Future<worker::Connection> future;
  };
  std::int32_t deployment_choice_ = 0;
  std::unique_ptr<worker::DeploymentList> deployment_list_;
  std::string queue_status_;
  std::string new_queue_status_;
  std::string queue_status_error_;

  std::unique_ptr<LocatorFutureWrapper> locator_future_;
  std::unique_ptr<ConnectionFutureWrapper> connection_future_;
};

}  // ::gloam

#endif
