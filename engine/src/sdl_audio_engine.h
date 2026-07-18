#ifndef CLASSIC_GAME_KIT_SDL_AUDIO_ENGINE_H_
#define CLASSIC_GAME_KIT_SDL_AUDIO_ENGINE_H_

#include "arcade_audio.h"

#include <SDL.h>

class SdlAudioEngine {
 public:
  SdlAudioEngine();
  ~SdlAudioEngine();

  bool Init();
  void Play(ArcadeSfx sfx);
  void SetEnabled(bool enabled);
  void Shutdown();

 private:
  static void AudioCallback(void* userdata, Uint8* stream, int len);
  void Render(Uint8* stream, int len);

  SDL_AudioDeviceID device_;
  ArcadeSynth synth_;
  bool enabled_;
  bool initialized_;
};

#endif  // CLASSIC_GAME_KIT_SDL_AUDIO_ENGINE_H_
