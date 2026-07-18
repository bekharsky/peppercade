#include "nacl_audio_engine.h"

#include <string.h>

namespace {
const uint32_t kRequestedFrames = 4096u;
const uint32_t kChannels = 2u;
const int kWarmupCallbacks = 3;
}

NaclAudioEngine::NaclAudioEngine()
    : sample_frame_count_(kRequestedFrames),
      enabled_(true),
      initialized_(false),
      playing_(false),
      active_after_callback_(false),
      warmup_callbacks_left_(0) {
  pthread_mutex_init(&mutex_, 0);
}

NaclAudioEngine::~NaclAudioEngine() {
  Shutdown();
  pthread_mutex_destroy(&mutex_);
}

bool NaclAudioEngine::Init(pp::Instance* instance) {
  if (!instance || initialized_) return initialized_;
  sample_frame_count_ = pp::AudioConfig::RecommendSampleFrameCount(
      instance, PP_AUDIOSAMPLERATE_44100, kRequestedFrames);
  audio_ = pp::Audio(
      instance,
      pp::AudioConfig(instance, PP_AUDIOSAMPLERATE_44100, sample_frame_count_),
      &NaclAudioEngine::AudioCallback, this);
  if (audio_.is_null()) return false;
  initialized_ = true;
  return initialized_;
}

void NaclAudioEngine::WarmUp() {
  if (!initialized_ || playing_) return;
  warmup_callbacks_left_ = kWarmupCallbacks;
  active_after_callback_ = false;
  playing_ = audio_.StartPlayback();
}

void NaclAudioEngine::Play(ArcadeSfx sfx) {
  if (!initialized_ || !enabled_) return;
  if (pthread_mutex_trylock(&mutex_) != 0) return;
  synth_.Play(sfx);
  active_after_callback_ = true;
  pthread_mutex_unlock(&mutex_);
  if (!playing_) playing_ = audio_.StartPlayback();
}

void NaclAudioEngine::SetEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  if (!initialized_) return;
  if (enabled_) {
    pthread_mutex_lock(&mutex_);
    active_after_callback_ = synth_.HasActivePlayback();
    pthread_mutex_unlock(&mutex_);
    if (active_after_callback_ && !playing_) {
      playing_ = audio_.StartPlayback();
    }
  } else {
    if (playing_) audio_.StopPlayback();
    playing_ = false;
    warmup_callbacks_left_ = 0;
    pthread_mutex_lock(&mutex_);
    synth_.Reset();
    active_after_callback_ = false;
    pthread_mutex_unlock(&mutex_);
  }
}

void NaclAudioEngine::Update() {
  if (!initialized_ || !enabled_ || !playing_) return;
  if (warmup_callbacks_left_ > 0) return;
  if (pthread_mutex_trylock(&mutex_) != 0) return;
  const bool active = active_after_callback_ || synth_.HasActivePlayback();
  pthread_mutex_unlock(&mutex_);
  if (!active) {
    audio_.StopPlayback();
    playing_ = false;
  }
}

void NaclAudioEngine::Shutdown() {
  if (!initialized_) return;
  if (playing_) audio_.StopPlayback();
  playing_ = false;
  warmup_callbacks_left_ = 0;
  initialized_ = false;
}

void NaclAudioEngine::AudioCallback(void* samples, uint32_t buffer_size,
                                    void* data) {
  reinterpret_cast<NaclAudioEngine*>(data)->Render(samples, buffer_size);
}

void NaclAudioEngine::Render(void* samples, uint32_t buffer_size) {
  memset(samples, 0, buffer_size);
  if (warmup_callbacks_left_ > 0) {
    --warmup_callbacks_left_;
    return;
  }
  if (!enabled_) return;
  if (pthread_mutex_trylock(&mutex_) != 0) return;
  uint32_t frames = buffer_size / (sizeof(int16_t) * kChannels);
  synth_.Render(reinterpret_cast<int16_t*>(samples), frames);
  active_after_callback_ = synth_.HasActivePlayback();
  pthread_mutex_unlock(&mutex_);
}
