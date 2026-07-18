#ifndef TIZEN_2048_GAME_2048_H_
#define TIZEN_2048_GAME_2048_H_

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

class Game2048 : public Game {
 public:
  Game2048();

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
  static const int kSize = 4;
  static const int kMaxParticles = 128;
  static const int kMaxBursts = 24;

  enum Direction {
    kDirLeft,
    kDirRight,
    kDirUp,
    kDirDown
  };

  struct MoveResult {
    bool moved;
    bool merged;
    int gained;
  };

  void Layout();
  void ClearBoard();
  void AddRandomTile();
  MoveResult Move(Direction direction);
  bool HasMoves() const;
  int BestTile() const;
  int PatternStageForTile(int value) const;
  int TileColorIndex(int value) const;
  void SpawnTileEffect(int col, int row, int color);
  void SpawnMergeEffect(int col, int row, int color);
  void UpdateEffects(double dt_ms);
  void QueueSound(ArcadeSfx sfx);
  void DrawBackground(Gles2Renderer* renderer);
  void DrawBoard(Gles2Renderer* renderer);
  void DrawTile(Gles2Renderer* renderer, int col, int row, int value);
  void DrawHud(Gles2Renderer* renderer);
  void DrawOverlay(Gles2Renderer* renderer);
  int width_;
  int height_;
  int board_x_;
  int board_y_;
  int board_size_;
  int cell_;
  int gap_;
  int hud_x_;
  int hud_y_;
  int hud_w_;
  int hud_h_;
  int score_;
  int moves_;
  int difficulty_;
  bool paused_;
  bool game_over_;
  bool won_;
  bool win_notice_;
  bool sound_muted_;
  int pattern_stage_;
  uint32_t sound_events_;
  uint32_t rng_;
  int tiles_[kSize][kSize];
  pixel_effects::Particle particles_[kMaxParticles];
  pixel_effects::Burst bursts_[kMaxBursts];
};

#endif  // TIZEN_2048_GAME_2048_H_
