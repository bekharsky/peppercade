#include "game_loop.h"

namespace {
const int kMaxStepsPerFrame = 5;
}

FixedStepClock::FixedStepClock(double step_ms)
    : step_ms_(step_ms),
      last_time_ms_(0.0),
      accumulator_ms_(0.0),
      interpolation_(0.0),
      last_frame_ms_(0.0),
      pending_steps_(0),
      dropped_steps_(0) {}

void FixedStepClock::Reset(double now_ms) {
  last_time_ms_ = now_ms;
  accumulator_ms_ = 0.0;
  interpolation_ = 0.0;
  last_frame_ms_ = 0.0;
  pending_steps_ = 0;
  dropped_steps_ = 0;
}

int FixedStepClock::BeginFrame(double now_ms) {
  double delta_ms = now_ms - last_time_ms_;
  if (delta_ms < 0.0) delta_ms = 0.0;
  if (delta_ms > 250.0) delta_ms = 250.0;
  last_time_ms_ = now_ms;
  last_frame_ms_ = delta_ms;

  accumulator_ms_ += delta_ms;
  pending_steps_ = static_cast<int>(accumulator_ms_ / step_ms_);
  if (pending_steps_ > kMaxStepsPerFrame) {
    dropped_steps_ += pending_steps_ - kMaxStepsPerFrame;
    pending_steps_ = kMaxStepsPerFrame;
    accumulator_ms_ = step_ms_ * kMaxStepsPerFrame;
  }
  interpolation_ = step_ms_ > 0.0 ? accumulator_ms_ / step_ms_ : 0.0;
  if (interpolation_ > 1.0) interpolation_ = 1.0;
  return pending_steps_;
}

void FixedStepClock::ConsumeStep() {
  if (pending_steps_ <= 0) return;
  accumulator_ms_ -= step_ms_;
  if (accumulator_ms_ < 0.0) accumulator_ms_ = 0.0;
  --pending_steps_;
  interpolation_ = step_ms_ > 0.0 ? accumulator_ms_ / step_ms_ : 0.0;
  if (interpolation_ > 1.0) interpolation_ = 1.0;
}
