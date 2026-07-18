#ifndef CLASSIC_GAME_KIT_ARCADE_AUDIO_H_
#define CLASSIC_GAME_KIT_ARCADE_AUDIO_H_

#include <stdint.h>

enum ArcadeSfx {
  kSfxPaddle = 0,
  kSfxBrick,
  kSfxBonus,
  kSfxLaser,
  kSfxLife,
  kSfxLevel,
  kSfxLose,
  kSfxPause,
  kSfxCount
};

namespace arcade {

// Event masks are intentionally separate from playback.  Games queue events;
// SDL/NaCl shells decide whether and how to play them.
inline void QueueSoundEvent(uint32_t* events, ArcadeSfx sfx) {
  if (events) *events |= 1u << static_cast<int>(sfx);
}

inline uint32_t ConsumeSoundEvents(uint32_t* events) {
  if (!events) return 0;
  const uint32_t pending = *events;
  *events = 0;
  return pending;
}

template <typename AudioEngine>
inline void PlaySoundEvents(uint32_t events, AudioEngine* audio) {
  if (!audio) return;
  for (int i = 0; i < kSfxCount; ++i)
    if (events & (1u << i)) audio->Play(static_cast<ArcadeSfx>(i));
}

}  // namespace arcade

class ArcadeSynth {
 public:
  ArcadeSynth();

  void Reset();
  void Play(ArcadeSfx sfx);
  void Render(int16_t* samples, uint32_t frame_count);
  bool HasActivePlayback() const;

 private:
  static const int kMaxSampleFrames = 22050;
  static const int kMaxPlaybacks = 6;

  struct Sample {
    int16_t data[kMaxSampleFrames];
    uint32_t frame_count;
  };

  struct Playback {
    bool active;
    int sample;
    uint32_t cursor;
  };

  void BuildSamples();
  void BuildSample(ArcadeSfx sfx, int wave, float frequency, float slide,
                   float volume, float duration_ms);
  void MixLayer(ArcadeSfx sfx, int wave, float frequency, float slide,
                float volume, float duration_ms);

  Sample samples_[kSfxCount];
  Playback playbacks_[kMaxPlaybacks];
};

#endif  // CLASSIC_GAME_KIT_ARCADE_AUDIO_H_
