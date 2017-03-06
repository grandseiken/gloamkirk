#ifndef GLOAM_CLIENT_SRC_TITLE_MODE_H
#define GLOAM_CLIENT_SRC_TITLE_MODE_H
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
  std::uint64_t frame_ = 0;
  std::uint64_t connect_frame_ = 0;
  std::uint64_t finish_connect_frame_ = 0;
  std::int32_t random_seed_ = 0;
  std::int32_t menu_item_ = MenuItem::kConnect;

  bool connection_local_;
  worker::ConnectionParameters connection_params_;
  worker::LocatorParameters locator_params_;
  // Workaround for broken future move-constructor.
  struct FutureWrapper {
    worker::Future<worker::Connection> future;
  };
  std::unique_ptr<FutureWrapper> connection_future_;
};

}  // ::gloam

#endif
