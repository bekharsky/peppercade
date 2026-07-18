#ifndef TIZEN_ARKANOID_GLES_GAME_H_
#define TIZEN_ARKANOID_GLES_GAME_H_

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

class ArkanoidGlesGame : public Game {
 public:
  ArkanoidGlesGame();

  void Resize(int width, int height);
  void Reset();
  void Update(double dt_ms, const InputState& input);
  void Render(Gles2Renderer* renderer);
  void TogglePauseFromSystem();
  void SetPaused(bool paused);
  void ToggleSoundMute();
  void CycleDifficulty();

  int score() const { return score_; }
  int level() const { return level_; }
  int lives() const { return lives_; }
  bool paused() const { return paused_; }
  bool game_over() const { return game_over_; }
  bool sound_muted() const { return sound_muted_; }
  uint32_t ConsumeSoundEvents();

 private:
  static const int kCols = 13;
  static const int kRows = 8;
  static const int kMaxBalls = 5;
  static const int kMaxBonuses = 10;
  static const int kMaxLasers = 4;
  static const int kMaxParticles = 180;
  static const int kMaxBursts = 24;

  struct Ball {
    float x;
    float y;
    float vx;
    float vy;
    bool active;
    bool stuck;
  };

  struct Brick {
    int hp;
    int color;
    int bonus;
  };

  struct Bonus {
    float x;
    float y;
    float vy;
    int type;
    bool active;
  };

  struct Laser {
    float x;
    float y;
    bool active;
  };

  void Layout();
  void GenerateLevel();
  void ServeBall();
  void LaunchBalls();
  void FireLaser();
  void MoveBall(Ball* ball, double dt);
  void MoveBallStep(Ball* ball, float dt, float slow);
  void HitBrick(int col, int row);
  void SpawnBonus(float x, float y, int type);
  void ApplyBonus(int type);
  void UpdateBonuses(double dt);
  void UpdateLasers(double dt);
  void LoseLife();
  void NextLevel();
  void SpawnParticles(float x, float y, int color, int count);
  void SpawnBurst(float x, float y, int color);
  void QueueSound(ArcadeSfx sfx);
  void UpdateEffects(double dt_ms);
  void DrawBackground(Gles2Renderer* renderer);
  void DrawPlayfield(Gles2Renderer* renderer);
  void DrawHud(Gles2Renderer* renderer);
  void DrawOverlay(Gles2Renderer* renderer);
  int width_;
  int height_;
  int play_x_;
  int play_y_;
  int play_w_;
  int play_h_;
  int hud_x_;
  int hud_y_;
  int hud_w_;
  int hud_h_;
  int border_;
  float paddle_x_;
  float paddle_y_;
  float paddle_w_;
  float paddle_h_;
  int score_;
  int lives_;
  int level_;
  int bricks_left_;
  bool paused_;
  bool game_over_;
  bool sound_muted_;
  int laser_charges_;
  float expand_ms_;
  float slow_ms_;
  uint32_t sound_events_;
  uint32_t rng_;
  Ball balls_[kMaxBalls];
  Brick bricks_[kRows][kCols];
  Bonus bonuses_[kMaxBonuses];
  Laser lasers_[kMaxLasers];
  pixel_effects::Particle particles_[kMaxParticles];
  pixel_effects::Burst bursts_[kMaxBursts];
};

#endif  // TIZEN_ARKANOID_GLES_GAME_H_
