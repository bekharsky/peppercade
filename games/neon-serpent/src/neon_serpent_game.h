#ifndef TIZEN_SNAKE_GAME_H_
#define TIZEN_SNAKE_GAME_H_

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

class SnakeGame : public Game {
 public:
  SnakeGame();

  void Reset();
  void Resize(int width, int height);
  void Update(double dt_ms, const InputState& input);
  void Render(Gles2Renderer* renderer);
  void SetPaused(bool paused);
  void ToggleSoundMute();
  void CycleDifficulty();

  bool paused() const { return paused_; }
  bool game_over() const { return game_over_; }
  bool sound_muted() const { return sound_muted_; }
  uint32_t ConsumeSoundEvents();

 private:
  static const int kCols = 28;
  static const int kRows = 20;
  static const int kMaxSegments = kCols * kRows;
  static const int kMaxParticles = 96;
  static const int kMaxBursts = 16;

  struct Cell {
    int x;
    int y;
  };

  void Layout();
  void SpawnFood();
  void Step();
  void QueueSound(ArcadeSfx sfx);
  void DrawBackground(Gles2Renderer* renderer);
  void DrawPlayfield(Gles2Renderer* renderer);
  void DrawHud(Gles2Renderer* renderer);
  void DrawOverlay(Gles2Renderer* renderer);
  bool Occupied(int x, int y) const;
  int width_;
  int height_;
  int play_x_;
  int play_y_;
  int play_w_;
  int play_h_;
  int cell_;
  int border_;
  int hud_x_;
  int hud_y_;
  int hud_w_;
  int hud_h_;
  int dir_x_;
  int dir_y_;
  int next_dir_x_;
  int next_dir_y_;
  int length_;
  int score_;
  int speed_;
  double step_ms_;
  double accumulator_ms_;
  bool paused_;
  bool game_over_;
  bool sound_muted_;
  uint32_t sound_events_;
  uint32_t rng_;
  Cell snake_[kMaxSegments];
  Cell food_;
  pixel_effects::Particle particles_[kMaxParticles];
  pixel_effects::Burst bursts_[kMaxBursts];
};

#endif  // TIZEN_SNAKE_GAME_H_
