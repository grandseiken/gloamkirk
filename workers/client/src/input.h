#ifndef GLOAM_WORKERS_CLIENT_SRC_INPUT_H
#define GLOAM_WORKERS_CLIENT_SRC_INPUT_H
#include <cstdint>
#include <memory>

namespace OIS {
class InputManager;
}  // ::OIS

namespace gloam {

class Input {
public:
  Input(std::size_t window_handle);

private:
  std::unique_ptr<OIS::InputManager, void (*)(OIS::InputManager*)> manager_;
};

}  // ::gloam

#endif
