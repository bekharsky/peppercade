#ifndef TIZEN_SOKOBAN_GAME_H_
#define TIZEN_SOKOBAN_GAME_H_

#include "engine/src/arcade_audio.h"
#include "engine/src/game.h"
#include "engine/src/gles2_renderer.h"
#include "engine/src/input_state.h"
#include "engine/src/pixel_effects.h"

#include <stdint.h>

class SokobanGame : public Game {
 public:
  SokobanGame();

  void Reset();
  void Resize(int width, int height);
  void Update(double dt_ms, const InputState& input);
  void Render(Gles2Renderer* renderer);
  void SetPaused(bool paused);
  void ToggleSoundMute();
  void CycleDifficulty();

  bool paused() const { return paused_; }
  bool game_over() const { return false; }
  bool sound_muted() const { return sound_muted_; }
  uint32_t ConsumeSoundEvents();

 private:
  static const int kCols = 30;
  static const int kRows = 17;
  static const int kMaxBoxes = 16;
  static const int kMaxParticles = 96;
  static const int kMaxBursts = 20;

  struct Cell {
    int x;
    int y;
  };

  struct Level {
    uint8_t walls[kRows][kCols];
    Cell boxes[kMaxBoxes];
    Cell goals[kMaxBoxes];
    Cell player;
    int box_count;
  };

  struct HatchingDragon {
    bool active;
    float x;
    float y;
    float vx;
    float vy;
  };

  void Layout();
  void GenerateLevel();
  void CenterLevel(Level* level) const;
  bool LoadCuratedLevel(Level* level, int index) const;
  bool VerifySolvable(const Level& level) const;
  bool TryMove(int dx, int dy);
  bool Solved() const;
  bool IsWall(int x, int y) const;
  int BoxAt(int x, int y) const;
  bool GoalAt(int x, int y) const;
  bool CellFree(const Level& level, int x, int y) const;
  int BoxAt(const Level& level, int x, int y) const;
  bool GoalsSatisfied(const Level& level) const;
  void QueueSound(ArcadeSfx sfx);
  void SpawnWallHit(int x, int y);
  void BeginHatching();
  void UpdateHatching(double dt_ms);
  void DrawBackground(Gles2Renderer* renderer);
  void DrawPatternPanel(Gles2Renderer* renderer, int x, int y, int w, int h,
                        int variant);
  void DrawBoard(Gles2Renderer* renderer);
  void DrawFloorTile(Gles2Renderer* renderer, int x, int y, bool has_fire);
  void DrawWallTile(Gles2Renderer* renderer, int x, int y, bool hit);
  void DrawGoal(Gles2Renderer* renderer, int x, int y);
  void DrawCrate(Gles2Renderer* renderer, int x, int y, bool stored);
  void DrawDragon(Gles2Renderer* renderer, const HatchingDragon& dragon);
  void DrawRobot(Gles2Renderer* renderer, int x, int y);
  void DrawHud(Gles2Renderer* renderer);
  void DrawOverlay(Gles2Renderer* renderer);
  void ActiveBounds(int* min_x, int* min_y, int* max_x, int* max_y) const;
  GlesRect TileRect(int x, int y, float inset = 0.0f) const;
  int width_;
  int height_;
  int board_x_;
  int board_y_;
  int board_w_;
  int board_h_;
  int cell_;
  int hud_x_;
  int hud_y_;
  int hud_w_;
  int hud_h_;
  int level_index_;
  int difficulty_;
  int moves_;
  int pushes_;
  bool paused_;
  bool solved_notice_;
  bool hatching_;
  double solved_timer_ms_;
  double hatch_timer_ms_;
  double wall_hit_ms_;
  double move_flash_ms_;
  double ambient_ms_;
  Cell wall_hit_;
  bool sound_muted_;
  uint32_t sound_events_;
  uint32_t rng_;
  Level level_;
  HatchingDragon dragons_[kMaxBoxes];
  pixel_effects::Particle particles_[kMaxParticles];
  pixel_effects::Burst bursts_[kMaxBursts];
};

#endif  // TIZEN_SOKOBAN_GAME_H_
