#include "arcade_audio.h"
#include "math_util.h"

#include <string.h>

namespace {

const float kSampleRate = 44100.0f;
const int kDecimate = 2;
const float kPi = 3.14159265358979323846f;
const float kTwoPi = kPi * 2.0f;

float Wave(int wave, float phase) {
  if (wave == 0) {
    float t = phase / kTwoPi;
    if (t < 0.25f) return t * 4.0f;
    if (t < 0.75f) return 2.0f - t * 4.0f;
    return t * 4.0f - 4.0f;
  }
  if (wave == 1) return phase < kPi ? 1.0f : -1.0f;
  if (wave == 2) return 1.0f - 2.0f * (phase / kTwoPi);
  return ((phase < kPi ? phase : kTwoPi - phase) / kPi) * 2.0f - 1.0f;
}

}  // namespace

ArcadeSynth::ArcadeSynth() {
  memset(samples_, 0, sizeof(samples_));
  BuildSamples();
  Reset();
}

void ArcadeSynth::Reset() { memset(playbacks_, 0, sizeof(playbacks_)); }

void ArcadeSynth::BuildSamples() {
  BuildSample(kSfxPaddle, 3, 260.0f, 600.0f, 0.20f, 55.0f);
  BuildSample(kSfxBrick, 1, 620.0f, -1200.0f, 0.23f, 90.0f);
  BuildSample(kSfxBonus, 2, 520.0f, 900.0f, 0.18f, 180.0f);
  MixLayer(kSfxBonus, 0, 1040.0f, -180.0f, 0.12f, 160.0f);
  BuildSample(kSfxLaser, 1, 980.0f, 2200.0f, 0.18f, 70.0f);
  BuildSample(kSfxLife, 0, 440.0f, 420.0f, 0.18f, 260.0f);
  MixLayer(kSfxLife, 0, 660.0f, 420.0f, 0.14f, 260.0f);
  BuildSample(kSfxLevel, 2, 380.0f, 980.0f, 0.20f, 320.0f);
  MixLayer(kSfxLevel, 2, 760.0f, 760.0f, 0.16f, 300.0f);
  BuildSample(kSfxLose, 2, 260.0f, -260.0f, 0.24f, 420.0f);
  BuildSample(kSfxPause, 3, 320.0f, 0.0f, 0.16f, 80.0f);
}

void ArcadeSynth::BuildSample(ArcadeSfx sfx, int wave, float frequency,
                              float slide, float volume, float duration_ms) {
  Sample& sample = samples_[sfx];
  memset(sample.data, 0, sizeof(sample.data));
  sample.frame_count = 0;
  MixLayer(sfx, wave, frequency, slide, volume, duration_ms);
}

void ArcadeSynth::MixLayer(ArcadeSfx sfx, int wave, float frequency,
                           float slide, float volume, float duration_ms) {
  Sample& sample = samples_[sfx];
  uint32_t frames = static_cast<uint32_t>(duration_ms * kSampleRate / 1000.0f);
  if (frames > kMaxSampleFrames) frames = kMaxSampleFrames;
  if (frames > sample.frame_count) sample.frame_count = frames;

  float phase = 0.0f;
  float held = 0.0f;
  int hold = 0;
  for (uint32_t i = 0; i < frames; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(frames);
    if (hold <= 0) {
      float envelope = (1.0f - t);
      envelope *= envelope;
      held = Wave(wave, phase) * volume * envelope;
      float hz = frequency + slide * t;
      if (hz < 40.0f) hz = 40.0f;
      phase += kTwoPi * hz * kDecimate / kSampleRate;
      while (phase >= kTwoPi) phase -= kTwoPi;
      hold = kDecimate;
    }
    int mixed = static_cast<int>(sample.data[i]) +
                static_cast<int>(held * 32767.0f);
    sample.data[i] = static_cast<int16_t>(arcade::ClampInt(mixed, -28000, 28000));
    --hold;
  }
}

void ArcadeSynth::Play(ArcadeSfx sfx) {
  if (sfx < 0 || sfx >= kSfxCount || samples_[sfx].frame_count == 0) return;
  int slot = -1;
  for (int i = 0; i < kMaxPlaybacks; ++i) {
    if (!playbacks_[i].active) {
      slot = i;
      break;
    }
  }
  if (slot < 0) slot = static_cast<int>(sfx) % kMaxPlaybacks;
  playbacks_[slot].active = true;
  playbacks_[slot].sample = static_cast<int>(sfx);
  playbacks_[slot].cursor = 0;
}

void ArcadeSynth::Render(int16_t* samples, uint32_t frame_count) {
  if (!samples) return;
  for (uint32_t i = 0; i < frame_count; ++i) {
    int mixed = 0;
    for (int p = 0; p < kMaxPlaybacks; ++p) {
      Playback& playback = playbacks_[p];
      if (!playback.active) continue;
      const Sample& sample = samples_[playback.sample];
      if (playback.cursor >= sample.frame_count) {
        playback.active = false;
        continue;
      }
      mixed += sample.data[playback.cursor++];
    }
    int16_t out = static_cast<int16_t>(arcade::ClampInt(mixed, -28000, 28000));
    *samples++ = out;
    *samples++ = out;
  }
}

bool ArcadeSynth::HasActivePlayback() const {
  for (int i = 0; i < kMaxPlaybacks; ++i) {
    if (playbacks_[i].active) return true;
  }
  return false;
}
