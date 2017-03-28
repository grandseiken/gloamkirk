#include "workers/client/src/input.h"
#include <OIS.h>
#include <SFML/Window.hpp>

namespace gloam {
namespace {

Button key_code_to_button(sf::Keyboard::Key code) {
  switch (code) {
  case sf::Keyboard::A:
  case sf::Keyboard::Left:
    return Button::kLeft;
  case sf::Keyboard::D:
  case sf::Keyboard::Right:
    return Button::kRight;
  case sf::Keyboard::S:
  case sf::Keyboard::Down:
    return Button::kDown;
  case sf::Keyboard::W:
  case sf::Keyboard::Up:
    return Button::kUp;
  case sf::Keyboard::Return:
    return Button::kConfirm;
  case sf::Keyboard::Escape:
    return Button::kCancel;
  default:
    return Button::kNone;
  }
}

}  // anonymous

// TODO: it's possible the input manager can only be used on the main thread.
Input::Input(std::size_t window_handle)
: manager_{OIS::InputManager::createInputSystem(window_handle),
           &OIS::InputManager::destroyInputSystem} {}

void Input::update() {
  any_key_ = false;
  pressed_.clear();
  released_.clear();
}

void Input::handle(const sf::Event& event) {
  if (event.type == sf::Event::KeyPressed) {
    any_key_ = true;

    auto button = key_code_to_button(event.key.code);
    if (button != Button::kNone) {
      pressed_.insert(button);
      held_.insert(button);
    }
  } else if (event.type == sf::Event::KeyReleased) {
    auto button = key_code_to_button(event.key.code);
    if (button != Button::kNone) {
      released_.insert(button);
      held_.erase(button);
    }
  }
}

bool Input::held(Button button) const {
  return held_.find(button) != held_.end();
}

bool Input::pressed(Button button) const {
  if (button == Button::kAnyKey) {
    return any_key_;
  }
  return pressed_.find(button) != pressed_.end();
}

bool Input::released(Button button) const {
  return released_.find(button) != released_.end();
}

}  // ::gloam
