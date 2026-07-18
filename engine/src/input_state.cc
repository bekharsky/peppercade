#include "input_state.h"

#include <string.h>

namespace {
uint32_t ActionBit(GameAction action) {
  return 1u << static_cast<int>(action);
}
}

InputState::InputState()
    : down_mask_(0), pressed_mask_(0), released_mask_(0) {
  memset(held_ms_, 0, sizeof(held_ms_));
}

void InputState::BeginFrame() {
  pressed_mask_ = 0;
  released_mask_ = 0;
}

void InputState::SetAction(GameAction action, bool down) {
  const uint32_t bit = ActionBit(action);
  const bool was_down = (down_mask_ & bit) != 0;
  if (down == was_down) return;
  if (down) {
    down_mask_ |= bit;
    pressed_mask_ |= bit;
    held_ms_[action] = 0.0;
  } else {
    down_mask_ &= ~bit;
    released_mask_ |= bit;
    held_ms_[action] = 0.0;
  }
}

void InputState::RepeatAction(GameAction action) {
  if (Down(action)) pressed_mask_ |= ActionBit(action);
}

bool InputState::Down(GameAction action) const {
  return (down_mask_ & ActionBit(action)) != 0;
}

bool InputState::Pressed(GameAction action) const {
  return (pressed_mask_ & ActionBit(action)) != 0;
}

bool InputState::Released(GameAction action) const {
  return (released_mask_ & ActionBit(action)) != 0;
}

double InputState::HeldMs(GameAction action) const {
  return held_ms_[action];
}

void InputState::Advance(double dt_ms) {
  for (int i = 0; i < kActionCount; ++i) {
    const GameAction action = static_cast<GameAction>(i);
    if (Down(action)) held_ms_[i] += dt_ms;
  }
}
