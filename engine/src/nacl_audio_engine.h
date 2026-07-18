#ifndef CLASSIC_GAME_KIT_NACL_AUDIO_ENGINE_H_
#define CLASSIC_GAME_KIT_NACL_AUDIO_ENGINE_H_

#include "arcade_audio.h"

#include <pthread.h>
#include <stdint.h>
#include <ppapi/cpp/audio.h>
#include <ppapi/cpp/audio_config.h>
#include <ppapi/cpp/instance.h>

class NaclAudioEngine {
 public:
  NaclAudioEngine();
  ~NaclAudioEngine();

  bool Init(pp::Instance* instance);
  void WarmUp();
  void Play(ArcadeSfx sfx);
  void SetEnabled(bool enabled);
  void Update();
  void Shutdown();

 private:
  static void AudioCallback(void* samples, uint32_t buffer_size, void* data);
  void Render(void* samples, uint32_t buffer_size);

  pp::Audio audio_;
  ArcadeSynth synth_;
  pthread_mutex_t mutex_;
  uint32_t sample_frame_count_;
  bool enabled_;
  bool initialized_;
  bool playing_;
  bool active_after_callback_;
  int warmup_callbacks_left_;
};

#endif  // CLASSIC_GAME_KIT_NACL_AUDIO_ENGINE_H_
