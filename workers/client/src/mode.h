#ifndef GLOAM_WORKERS_CLIENT_SRC_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_MODE_H
#include <cstdint>
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
  kFramerate,
  kAntialiasLevel,
  kBack,
  kCount,
};

enum class Framerate {
  k30Fps,
  k60Fps,
  kCount,
};

class Mode;
struct ModeState {
  std::unique_ptr<Mode> new_mode;
  bool exit_application = false;
  bool exit_to_title = false;

  std::string worker_id;
  std::int32_t random_seed = 0;
  std::uint64_t frame = 0;
  double client_load = 0.;

  // Main menu.
  MenuItem menu_item = MenuItem::kConnect;
  SettingsItem settings_item = SettingsItem::kToggleFullscreen;
  bool settings_menu = false;

  // Command-line arguments.
  bool connect_local = false;
  bool enable_protocol_logging = false;
  std::string login_token;

  // Settings.
  bool fullscreen = false;
  bool antialiasing = true;
  Framerate framerate = Framerate::k60Fps;
};

class Mode {
public:
  virtual ~Mode() = default;
  virtual void tick(const Input& input) = 0;
  virtual void sync() = 0;
  virtual void render(const Renderer& renderer) const = 0;
};

}  // ::gloam

#endif
