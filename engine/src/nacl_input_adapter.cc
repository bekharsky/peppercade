#include "nacl_input_adapter.h"

NaclInputAdapter::NaclInputAdapter(InputState* input) : input_(input) {}

bool NaclInputAdapter::HandleInputEvent(const pp::InputEvent& event) {
  const PP_InputEvent_Type type = event.GetType();
  if (type != PP_INPUTEVENT_TYPE_KEYDOWN &&
      type != PP_INPUTEVENT_TYPE_RAWKEYDOWN &&
      type != PP_INPUTEVENT_TYPE_KEYUP) {
    return false;
  }
  pp::KeyboardInputEvent key(event);
  return SetKey(key.GetKeyCode(), type != PP_INPUTEVENT_TYPE_KEYUP);
}

bool NaclInputAdapter::SetKey(int key_code, bool down) {
  if (!input_) return false;
  switch (key_code) {
    case 37:
    case 65361:
      input_->SetAction(kActionLeft, down);
      return true;
    case 38:
    case 65362:
      input_->SetAction(kActionUp, down);
      return true;
    case 39:
    case 65363:
      input_->SetAction(kActionRight, down);
      return true;
    case 40:
    case 65364:
      input_->SetAction(kActionDown, down);
      return true;
    case 13:
    case 65293:
      input_->SetAction(kActionPrimary, down);
      return true;
    case 8:
    case 461:
    case 10009:
    case 19:
    case 65307:
      input_->SetAction(kActionBack, down);
      return true;
    case 65:
      input_->SetAction(kActionPrimary, down);
      return true;
    case 66:
      input_->SetAction(kActionSecondary, down);
      return true;
    case 32:
      input_->SetAction(kActionPrimary, down);
      return true;
    default:
      return false;
  }
}
