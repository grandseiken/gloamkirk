#ifndef GLOAM_WORKERS_CLIENT_SRC_INPUT_H
#define GLOAM_WORKERS_CLIENT_SRC_INPUT_H
#include "common/src/common/hashes.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace OIS {
class InputManager;
}  // ::OIS

namespace sf {
class Event;
}

namespace gloam {
enum class Button {
  kNone,
  kAnyKey,
  kConfirm,
  kCancel,
  kLeft,
  kRight,
  kDown,
  kUp,
};

class Input {
public:
  Input(std::size_t window_handle);

  // Called at the start of each frame.
  void update();
  // Called after input() for each event since the last frame.
  void handle(const sf::Event& event);

  bool held(Button button) const;
  bool pressed(Button button) const;
  bool released(Button button) const;

private:
  std::unique_ptr<OIS::InputManager, void (*)(OIS::InputManager*)> manager_;

  bool any_key_ = false;
  std::unordered_set<Button, common::identity_hash<Button>> held_;
  std::unordered_set<Button, common::identity_hash<Button>> pressed_;
  std::unordered_set<Button, common::identity_hash<Button>> released_;
};

}  // ::gloam

#endif
