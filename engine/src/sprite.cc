#include "sprite.h"

SpriteFrame::SpriteFrame()
    : source(GlesRect{0.0f, 0.0f, 0.0f, 0.0f}), duration_ms(0.0) {}

SpriteFrame::SpriteFrame(GlesRect source_rect, double duration)
    : source(source_rect), duration_ms(duration) {}

SpriteAnimation::SpriteAnimation() : total_duration_ms_(0.0) {}

void SpriteAnimation::Clear() {
  frames_.clear();
  total_duration_ms_ = 0.0;
}

void SpriteAnimation::AddFrame(GlesRect source_rect, double duration_ms) {
  if (duration_ms < 0.0) duration_ms = 0.0;
  frames_.push_back(SpriteFrame(source_rect, duration_ms));
  total_duration_ms_ += duration_ms;
}

const SpriteFrame& SpriteAnimation::frame(size_t index) const {
  return frames_[index];
}

SpriteAnimator::SpriteAnimator()
    : animation_(0),
      frame_index_(0),
      frame_time_ms_(0.0),
      loop_(true),
      finished_(false) {}

void SpriteAnimator::SetAnimation(const SpriteAnimation* animation, bool loop) {
  animation_ = animation;
  loop_ = loop;
  Reset();
}

void SpriteAnimator::Reset() {
  frame_index_ = 0;
  frame_time_ms_ = 0.0;
  finished_ = false;
}

void SpriteAnimator::Advance(double dt_ms) {
  if (!animation_ || animation_->empty() || finished_) return;
  if (dt_ms < 0.0) dt_ms = 0.0;
  frame_time_ms_ += dt_ms;

  while (!finished_ && frame_time_ms_ >= animation_->frame(frame_index_).duration_ms) {
    const double duration = animation_->frame(frame_index_).duration_ms;
    if (duration > 0.0) frame_time_ms_ -= duration;
    ++frame_index_;
    if (frame_index_ < animation_->frame_count()) continue;
    if (loop_) {
      frame_index_ = 0;
    } else {
      frame_index_ = animation_->frame_count() - 1;
      finished_ = true;
    }
  }
}

const SpriteFrame* SpriteAnimator::CurrentFrame() const {
  if (!animation_ || animation_->empty()) return 0;
  return &animation_->frame(frame_index_);
}

SpriteDraw::SpriteDraw()
    : texture(0),
      source(GlesRect{0.0f, 0.0f, 0.0f, 0.0f}),
      destination(GlesRect{0.0f, 0.0f, 0.0f, 0.0f}),
      tint(GlesColor{1.0f, 1.0f, 1.0f, 1.0f}),
      visible(true) {}

void DrawSprite(Gles2Renderer* renderer, const SpriteDraw& sprite) {
  if (!renderer || !sprite.texture || !sprite.visible) return;
  renderer->DrawTexture(*sprite.texture, sprite.destination, sprite.source,
                        sprite.tint);
}
