#ifndef TIZEN_BUBBLE_SHOOTER_GAME_H_
#define TIZEN_BUBBLE_SHOOTER_GAME_H_

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

class BubbleShooterGame : public Game {
 public:
  BubbleShooterGame();

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
  static const int kCols = 14;
  static const int kRows = 12;
  static const int kColors = 6;
  static const int kMaxParticles = 256;
  static const int kMaxFlyingBubbles = 128;
  static const int kRowShiftDurationMs = 280;

  struct FlyingBubble {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    int color;
    bool active;
  };

  void Layout();
  void NewLevel();
  void Shoot();
  void LockShot();
  void AddRow();
  int DropUnsupportedBubbles();
  int RandomActiveColor();
  void SpawnPop(float x, float y, int color, int count);
  void SpawnFlyingBubble(float x, float y, int color, bool detached);
  void UpdateFlyingBubbles(double dt_ms);
  void DrawFlyingBubbles(Gles2Renderer* renderer);
  void QueueSound(ArcadeSfx sfx);
  void DrawBackground(Gles2Renderer* renderer);
  void DrawBoard(Gles2Renderer* renderer);
  void DrawHud(Gles2Renderer* renderer);
  void DrawOverlay(Gles2Renderer* renderer);
  void DrawBubble(Gles2Renderer* renderer, float x, float y, float radius,
                  int color);
  int NearestRow(float y) const;
  int NearestCol(float x, int row) const;
  float BubbleX(int row, int col) const;
  float BubbleY(int row) const;
  float PlayInset() const;
  bool FindAttachCell(float x, float y, int* row, int* col) const;
  bool HasBubbleNeighbor(int row, int col) const;
  bool InBounds(int row, int col) const;
  int width_, height_, board_x_, board_y_, board_w_, board_h_, hud_x_, hud_w_;
  float radius_, aim_angle_, shot_x_, shot_y_, shot_vx_, shot_vy_;
  double row_shift_ms_;
  int level_, score_, misses_, shot_color_, next_color_, speed_;
  bool shot_active_, paused_, game_over_, win_notice_, sound_muted_;
  uint32_t sound_events_, rng_;
  int bubbles_[kRows][kCols];
  pixel_effects::Particle particles_[kMaxParticles];
  FlyingBubble flying_bubbles_[kMaxFlyingBubbles];
};

#endif  // TIZEN_BUBBLE_SHOOTER_GAME_H_
