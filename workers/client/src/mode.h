#ifndef GLOAM_CLIENT_SRC_MODE_H
#define GLOAM_CLIENT_SRC_MODE_H
#include <memory>

namespace sf {
class Event;
}  // ::sf

namespace gloam {
class Renderer;

enum class ModeAction {
  kNone,
  kToggleFullscreen,
  kExitApplication,
};

class Mode {
public:
  virtual ~Mode() = default;
  virtual ModeAction event(const sf::Event& event) = 0;
  virtual std::unique_ptr<Mode> update() = 0;
  virtual void render(const Renderer& renderer) const = 0;
};

}  // ::gloam

#endif
