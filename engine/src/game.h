#ifndef CLASSIC_GAME_KIT_GAME_H_
#define CLASSIC_GAME_KIT_GAME_H_

#include "arcade_audio.h"
#include "gles2_renderer.h"
#include "input_state.h"

#include <stdint.h>

// Platform-neutral surface implemented by every game.  Shells own platform
// setup, input delivery, audio playback, and game selection through this API.
class Game {
 public:
  virtual ~Game() {}

  virtual void Reset() = 0;
  virtual void Resize(int width, int height) = 0;
  virtual void Update(double dt_ms, const InputState& input) = 0;
  virtual void Render(Gles2Renderer* renderer) = 0;
  virtual void SetPaused(bool paused) = 0;
  virtual void ToggleSoundMute() = 0;
  virtual void CycleDifficulty() = 0;

  virtual bool paused() const = 0;
  virtual bool game_over() const = 0;
  virtual bool sound_muted() const = 0;
  virtual uint32_t ConsumeSoundEvents() = 0;
};

#endif  // CLASSIC_GAME_KIT_GAME_H_
