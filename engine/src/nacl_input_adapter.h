#ifndef CLASSIC_GAME_KIT_NACL_INPUT_ADAPTER_H_
#define CLASSIC_GAME_KIT_NACL_INPUT_ADAPTER_H_

#include "input_state.h"

#include "ppapi/cpp/input_event.h"

class NaclInputAdapter {
 public:
  explicit NaclInputAdapter(InputState* input);

  bool HandleInputEvent(const pp::InputEvent& event);

 private:
  bool SetKey(int key_code, bool down);

  InputState* input_;
};

#endif  // CLASSIC_GAME_KIT_NACL_INPUT_ADAPTER_H_
