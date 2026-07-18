#ifndef CLASSIC_GAME_KIT_INPUT_STATE_H_
#define CLASSIC_GAME_KIT_INPUT_STATE_H_

#include <stdint.h>

enum GameAction {
  kActionLeft = 0,
  kActionRight,
  kActionUp,
  kActionDown,
  kActionPrimary,
  kActionSecondary,
  kActionPause,
  kActionBack,
  kActionCount
};

class InputState {
 public:
  InputState();

  void BeginFrame();
  void SetAction(GameAction action, bool down);
  void RepeatAction(GameAction action);

  bool Down(GameAction action) const;
  bool Pressed(GameAction action) const;
  bool Released(GameAction action) const;
  double HeldMs(GameAction action) const;

  void Advance(double dt_ms);

 private:
  uint32_t down_mask_;
  uint32_t pressed_mask_;
  uint32_t released_mask_;
  double held_ms_[kActionCount];
};

#endif  // CLASSIC_GAME_KIT_INPUT_STATE_H_
