#ifndef GLOAM_WORKERS_CLIENT_SRC_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_MODE_H
#include <memory>

namespace sf {
class Event;
}  // ::sf

namespace gloam {
class Input;
class Renderer;

enum class ModeAction {
  kNone,
  kToggleFullscreen,
  kExitToTitle,
  kExitApplication,
};

class Mode;
struct ModeResult {
  ModeAction action;
  std::unique_ptr<Mode> new_mode;
};

class Mode {
public:
  virtual ~Mode() = default;
  virtual ModeResult update(const Input& input) = 0;
  virtual void render(const Renderer& renderer) const = 0;
};

}  // ::gloam

#endif
