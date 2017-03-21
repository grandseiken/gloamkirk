#ifndef GLOAM_WORKERS_CLIENT_SRC_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_MODE_H
#include <memory>
#include <string>

namespace sf {
class Event;
}  // ::sf

namespace gloam {
class Input;
class Renderer;

enum class MenuItem {
  kConnect,
  kSettings,
  kExitApplication,
  kCount,
};

enum class SettingsItem {
  kToggleFullscreen,
  kAntialiasLevel,
  kBack,
  kCount,
};

class Mode;
struct ModeState {
  std::unique_ptr<Mode> new_mode;
  bool exit_application = false;
  bool exit_to_title = false;

  std::int32_t random_seed = 0;
  std::uint64_t frame = 0;

  // Main menu.
  MenuItem menu_item = MenuItem::kConnect;
  SettingsItem settings_item = SettingsItem::kToggleFullscreen;
  bool settings_menu = false;

  // Command-line arguments.
  bool connect_local = false;
  std::string login_token;

  // Settings.
  bool fullscreen = false;
  bool antialiasing = true;
};

class Mode {
public:
  virtual ~Mode() = default;
  virtual void update(const Input& input) = 0;
  virtual void render(const Renderer& renderer) const = 0;
};

}  // ::gloam

#endif
