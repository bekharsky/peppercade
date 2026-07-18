#include "sdl_audio_engine.h"

#include <string.h>

namespace {
const int kChannels = 2;
}

SdlAudioEngine::SdlAudioEngine()
    : device_(0), enabled_(true), initialized_(false) {}

SdlAudioEngine::~SdlAudioEngine() { Shutdown(); }

bool SdlAudioEngine::Init() {
  if (initialized_) return true;
  SDL_AudioSpec want;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = kChannels;
  want.samples = 2048;
  want.callback = &SdlAudioEngine::AudioCallback;
  want.userdata = this;
  device_ = SDL_OpenAudioDevice(0, 0, &want, 0, 0);
  if (!device_) return false;
  initialized_ = true;
  return true;
}

void SdlAudioEngine::Play(ArcadeSfx sfx) {
  if (!initialized_ || !enabled_) return;
  SDL_LockAudioDevice(device_);
  synth_.Play(sfx);
  SDL_UnlockAudioDevice(device_);
  SDL_PauseAudioDevice(device_, 0);
}

void SdlAudioEngine::SetEnabled(bool enabled) {
  enabled_ = enabled;
  if (!initialized_) return;
  if (!enabled_) {
    SDL_PauseAudioDevice(device_, 1);
    SDL_LockAudioDevice(device_);
    synth_.Reset();
    SDL_UnlockAudioDevice(device_);
  }
}

void SdlAudioEngine::Shutdown() {
  if (!initialized_) return;
  SDL_CloseAudioDevice(device_);
  device_ = 0;
  initialized_ = false;
}

void SdlAudioEngine::AudioCallback(void* userdata, Uint8* stream, int len) {
  reinterpret_cast<SdlAudioEngine*>(userdata)->Render(stream, len);
}

void SdlAudioEngine::Render(Uint8* stream, int len) {
  memset(stream, 0, len);
  if (!enabled_) return;
  synth_.Render(reinterpret_cast<int16_t*>(stream),
                static_cast<uint32_t>(len / (sizeof(int16_t) * kChannels)));
}
