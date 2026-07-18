#ifndef TIZEN_TETRIS_CORE_H_
#define TIZEN_TETRIS_CORE_H_

#include "stackfall_patterns.h"

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

enum TetrisKey {
  TETRIS_KEY_LEFT,
  TETRIS_KEY_RIGHT,
  TETRIS_KEY_UP,
  TETRIS_KEY_DOWN,
  TETRIS_KEY_ROTATE,
  TETRIS_KEY_BACK,
  TETRIS_KEY_CONFIRM
};

class TetrisGame : public Game {
 public:
  static const int kPieceCount = 7;

  TetrisGame();

  void Reset();
  void Resize(int width, int height);
  void Update(double dt_ms, const InputState& input);
  bool Tick(int elapsed_ms);
  void KeyDown(TetrisKey key);
  void KeyUp(TetrisKey key);
  void Render(Gles2Renderer* renderer);
  bool paused() const { return paused_; }
  bool game_over() const { return game_over_; }
  bool sound_muted() const { return sound_muted_; }
  void SetPaused(bool paused);
  void CycleDifficulty();
  void ToggleSoundMute();
  uint32_t ConsumeSoundEvents();

 private:
  static const int kCols = 10;
  static const int kRows = 20;
  static const int kMaxParticles = 160;
  static const int kMaxBursts = 20;
  struct Piece {
    int type;
    int x;
    int y;
    uint8_t cells[4][4];
  };

  void Layout();
  void RefillBag();
  int TakeBag();
  void CopyShape(int type, uint8_t out[4][4]);
  void ShapeTopLeft(const uint8_t cells[4][4], int* left, int* top) const;
  void Spawn();
  bool Collides(const Piece& p, int dx, int dy, const uint8_t cells[4][4]);
  void Move(int dx);
  void RepeatHorizontalMove(int elapsed_ms);
  void Rotate();
  void SoftDrop();
  void StartStepDrop(int rows);
  bool AnimateStepDrop(int elapsed_ms);
  void Drop();
  void Merge();
  void StartLineClear();
  void FinishLineClear();
  void SpawnCellEffect(int gx, int gy, int type, int count);
  void SpawnLineEffect(int row);
  void UpdateEffects(double dt_ms);
  void RecomputeDropSpeed();
  void AdjustPausedSpeed(int delta_ms);
  void TogglePause();
  void QueueSound(ArcadeSfx sfx);

  void FillRect(int x, int y, int w, int h, TetrisColor c);
  void OutlineRect(int x, int y, int w, int h, TetrisColor c, int t);
  void DrawBlockPixels(int x, int y, int size, int type);
  void DrawBlock(int gx, int gy, int type);
  void DrawBoard();
  void DrawEffects();
  bool IsClearingRow(int row) const;
  void DrawDigit(int x, int y, int s, int value, TetrisColor on);
  void DrawNumber(int x, int y, int s, int value, int digits, TetrisColor on);
  void DrawMiniPiece(int x, int y, int box);
  void DrawHud();
  void HudRect(int* x, int* y, int* w, int* h) const;
  void DrawPatterns();
  void DrawPatternPanel(int x, int y, int w, int h, int variant);
  void DrawOverlay();

  Gles2Renderer* renderer_;
  int width_;
  int height_;
  int cell_;
  int board_x_;
  int board_y_;
  int board_w_;
  int board_h_;
  int board_border_;
  int left_w_;
  int right_x_;
  int right_w_;
  int lines_;
  int score_;
  int pattern_index_;
  int player_drop_ms_;
  int drop_ms_;
  int drop_accum_;
  int horizontal_dir_;
  int horizontal_accum_;
  int horizontal_wait_ms_;
  int step_drop_cells_;
  int step_drop_px_;
  int step_drop_subpx_;
  int clear_elapsed_ms_;
  int clear_rows_[kRows];
  int clear_count_;
  uint32_t sound_events_;
  bool down_held_;
  bool left_held_;
  bool right_held_;
  bool clearing_;
  bool paused_;
  bool game_over_;
  bool sound_muted_;
  int grid_[kRows][kCols];
  Piece current_;
  int next_type_;
  int bag_[kPieceCount];
  int bag_size_;
  uint32_t rng_;
  pixel_effects::Particle particles_[kMaxParticles];
  pixel_effects::Burst bursts_[kMaxBursts];
};

#endif  // TIZEN_TETRIS_CORE_H_
