#include "workers/client/src/input.h"
#include "OIS.h"

namespace gloam {

Input::Input(std::size_t window_handle)
: manager_{OIS::InputManager::createInputSystem(window_handle),
           &OIS::InputManager::destroyInputSystem} {}

}  // ::gloam
