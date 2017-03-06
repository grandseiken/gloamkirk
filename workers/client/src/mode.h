#ifndef GLOAM_CLIENT_SRC_MODE_H
#define GLOAM_CLIENT_SRC_MODE_H

namespace sf {
class Event;
}  // ::sf

namespace gloam {
class Renderer;

enum class ModeAction {
  kNone,
  kConnect,
  kToggleFullscreen,
  kExitApplication,
};

class Mode {
public:
  virtual ~Mode() = default;
  virtual ModeAction event(const sf::Event& event) = 0;
  virtual void update() = 0;
  virtual void render(const Renderer& renderer) const = 0;
};

}  // ::gloam

#endif
