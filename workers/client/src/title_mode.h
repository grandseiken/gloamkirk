#ifndef GLOAM_CLIENT_SRC_TITLE_MODE_H
#define GLOAM_CLIENT_SRC_TITLE_MODE_H
#include "workers/client/src/glo.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/renderer.h"
#include <cstdint>

namespace gloam {

class TitleMode : public Mode {
public:
  TitleMode(bool first_run);
  ModeAction event(const sf::Event& event) override;
  void update() override;
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

  std::uint64_t frame_ = 0;
  std::int32_t random_seed_ = 0;
  std::int32_t menu_item_ = MenuItem::kConnect;
};

}  // ::gloam

#endif
