#include "workers/client/src/input.h"
#include <OIS.h>
#include <SFML/Window.hpp>

namespace gloam {
namespace {

Button key_code_to_button(sf::Keyboard::Key code) {
  switch (code) {
  case sf::Keyboard::Left:
    return Button::kLeft;
  case sf::Keyboard::Right:
    return Button::kRight;
  case sf::Keyboard::Down:
    return Button::kDown;
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
      held_.emplace(button, 0);
      ++held_[button];
    }
  } else if (event.type == sf::Event::KeyReleased) {
    auto button = key_code_to_button(event.key.code);
    if (button != Button::kNone) {
      released_.insert(button);
      auto it = held_.find(button);
      if (it != held_.end() && it->second && !--it->second) {
        held_.erase(it);
      }
    }
  }
}

bool Input::held(Button button) const {
  auto it = held_.find(button);
  return it != held_.end() && it->second;
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
