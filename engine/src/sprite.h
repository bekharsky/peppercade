#ifndef CLASSIC_GAME_KIT_SPRITE_H_
#define CLASSIC_GAME_KIT_SPRITE_H_

#include "gles2_renderer.h"

#include <stddef.h>
#include <vector>

struct SpriteFrame {
  SpriteFrame();
  SpriteFrame(GlesRect source_rect, double duration_ms);

  GlesRect source;
  double duration_ms;
};

class SpriteAnimation {
 public:
  SpriteAnimation();

  void Clear();
  void AddFrame(GlesRect source_rect, double duration_ms);
  const SpriteFrame& frame(size_t index) const;
  size_t frame_count() const { return frames_.size(); }
  bool empty() const { return frames_.empty(); }
  double total_duration_ms() const { return total_duration_ms_; }

 private:
  std::vector<SpriteFrame> frames_;
  double total_duration_ms_;
};

class SpriteAnimator {
 public:
  SpriteAnimator();

  void SetAnimation(const SpriteAnimation* animation, bool loop);
  void Reset();
  void Advance(double dt_ms);
  const SpriteFrame* CurrentFrame() const;
  size_t frame_index() const { return frame_index_; }
  bool finished() const { return finished_; }

 private:
  const SpriteAnimation* animation_;
  size_t frame_index_;
  double frame_time_ms_;
  bool loop_;
  bool finished_;
};

struct SpriteDraw {
  SpriteDraw();

  const GlesTexture* texture;
  GlesRect source;
  GlesRect destination;
  GlesColor tint;
  bool visible;
};

void DrawSprite(Gles2Renderer* renderer, const SpriteDraw& sprite);

#endif  // CLASSIC_GAME_KIT_SPRITE_H_
