#ifndef CLASSIC_GAME_KIT_GAME_LOOP_H_
#define CLASSIC_GAME_KIT_GAME_LOOP_H_

class FixedStepClock {
 public:
  explicit FixedStepClock(double step_ms);

  void Reset(double now_ms);
  int BeginFrame(double now_ms);
  void ConsumeStep();

  double step_ms() const { return step_ms_; }
  double step_seconds() const { return step_ms_ / 1000.0; }
  double interpolation() const { return interpolation_; }
  double last_frame_ms() const { return last_frame_ms_; }
  int pending_steps() const { return pending_steps_; }
  int dropped_steps() const { return dropped_steps_; }

 private:
  double step_ms_;
  double last_time_ms_;
  double accumulator_ms_;
  double interpolation_;
  double last_frame_ms_;
  int pending_steps_;
  int dropped_steps_;
};

#endif  // CLASSIC_GAME_KIT_GAME_LOOP_H_
