#ifndef TIZEN_TRON_GAME_H_
#define TIZEN_TRON_GAME_H_

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

class TronGame : public Game {
 public:
  TronGame();

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
  static const int kCols = 48;
  static const int kRows = 30;
  static const int kMaxBursts = 16;
  static const int kMaxOpponents = 5;

  struct Bike {
    int x;
    int y;
    int dx;
    int dy;
    int color;
    bool alive;
  };

  void Layout();
  void ClearGrid();
  void Step();
  void SteerPlayer(const InputState& input);
  void SteerAi(int index);
  int OpponentCountForLevel() const;
  int CountReachableCells(int start_x, int start_y) const;
  bool CanMove(int x, int y) const;
  void QueueSound(ArcadeSfx sfx);
  void DrawBackground(Gles2Renderer* renderer);
  void DrawArena(Gles2Renderer* renderer);
  void DrawHud(Gles2Renderer* renderer);
  void DrawOverlay(Gles2Renderer* renderer);
  int width_;
  int height_;
  int arena_x_;
  int arena_y_;
  int arena_w_;
  int arena_h_;
  int cell_;
  int border_;
  int hud_x_;
  int hud_y_;
  int hud_w_;
  int hud_h_;
  int score_;
  int rounds_;
  int speed_;
  double step_ms_;
  double accumulator_ms_;
  bool paused_;
  bool game_over_;
  bool player_won_;
  bool sound_muted_;
  uint32_t sound_events_;
  uint32_t rng_;
  uint8_t grid_[kRows][kCols];
  Bike player_;
  Bike opponents_[kMaxOpponents];
  int opponent_count_;
  pixel_effects::Burst bursts_[kMaxBursts];
};

#endif  // TIZEN_TRON_GAME_H_
