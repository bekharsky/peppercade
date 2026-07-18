#include "stackfall_core.h"
#include "stackfall_assets.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/color_util.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"
#include "engine/src/pixel_effects.h"

#include "stackfall_patterns.h"

#include <string.h>

namespace {

enum PieceType { I, J, L, O, S, T, Z };

const uint8_t kShapes[TetrisGame::kPieceCount][4][4] = {
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
};

GlesColor ToGles(TetrisColor c) {
  return arcade::Rgb(c.r, c.g, c.b);
}

}  // namespace

TetrisGame::TetrisGame()
    : renderer_(0),
      width_(1280),
      height_(720),
      cell_(36),
      board_x_(0),
      board_y_(0),
      board_w_(0),
      board_h_(0),
      board_border_(5),
      left_w_(0),
      right_x_(0),
      right_w_(0),
      lines_(0),
      score_(0),
      pattern_index_(0),
      player_drop_ms_(360),
      drop_ms_(360),
      drop_accum_(0),
      horizontal_dir_(0),
      horizontal_accum_(0),
      horizontal_wait_ms_(0),
      step_drop_cells_(0),
      step_drop_px_(0),
      step_drop_subpx_(0),
      clear_elapsed_ms_(0),
      clear_count_(0),
      sound_events_(0),
      down_held_(false),
      left_held_(false),
      right_held_(false),
      clearing_(false),
      paused_(false),
      game_over_(false),
      sound_muted_(false),
      next_type_(0),
      bag_size_(0),
      rng_(0x4d595df4u) {
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  Reset();
}

void TetrisGame::Reset() {
  memset(grid_, -1, sizeof(grid_));
  lines_ = 0;
  score_ = 0;
  pattern_index_ = 0;
  player_drop_ms_ = 360;
  RecomputeDropSpeed();
  drop_accum_ = 0;
  horizontal_dir_ = 0;
  horizontal_accum_ = 0;
  horizontal_wait_ms_ = 0;
  step_drop_cells_ = 0;
  step_drop_px_ = 0;
  step_drop_subpx_ = 0;
  clear_elapsed_ms_ = 0;
  clear_count_ = 0;
  sound_events_ = 0;
  down_held_ = false;
  left_held_ = false;
  right_held_ = false;
  clearing_ = false;
  paused_ = false;
  game_over_ = false;
  bag_size_ = 0;
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  RefillBag();
  next_type_ = TakeBag();
  Spawn();
  Layout();
}

void TetrisGame::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  Layout();
}

void TetrisGame::Update(double dt_ms, const InputState& input) {
  if (input.Pressed(kActionBack) || input.Pressed(kActionPause))
    KeyDown(TETRIS_KEY_BACK);
  if (input.Pressed(kActionLeft)) KeyDown(TETRIS_KEY_LEFT);
  if (input.Released(kActionLeft)) KeyUp(TETRIS_KEY_LEFT);
  if (input.Pressed(kActionRight)) KeyDown(TETRIS_KEY_RIGHT);
  if (input.Released(kActionRight)) KeyUp(TETRIS_KEY_RIGHT);
  if (input.Pressed(kActionDown)) KeyDown(TETRIS_KEY_DOWN);
  if (input.Released(kActionDown)) KeyUp(TETRIS_KEY_DOWN);
  if (input.Pressed(kActionUp)) KeyDown(TETRIS_KEY_UP);
  if (input.Pressed(kActionPrimary)) KeyDown(TETRIS_KEY_CONFIRM);
  if (input.Pressed(kActionSecondary)) KeyDown(TETRIS_KEY_ROTATE);
  Tick(static_cast<int>(dt_ms + 0.5));
}

void TetrisGame::Layout() {
  int margin = arcade::ScreenMargin(width_, height_);
  int gap = arcade::SideHudGap(width_);
  int reserved_hud = arcade::SideHudWidth(width_);
  board_border_ = arcade::PanelBorderSize(reserved_hud);
  int avail_w = width_ - margin * 2 - gap - reserved_hud;
  int avail_h = height_ - margin * 2;
  cell_ = arcade::MinInt(avail_w / kCols, avail_h / kRows);
  cell_ = arcade::MaxInt(6, cell_);
  board_h_ = cell_ * kRows + board_border_ * 2;
  board_w_ = cell_ * kCols + board_border_ * 2;
  int group_w = board_w_ + gap + reserved_hud;
  board_x_ = (width_ - group_w) / 2;
  board_y_ = (height_ - board_h_) / 2;
  left_w_ = arcade::MaxInt(0, board_x_);
  right_x_ = board_x_ + board_w_;
  right_w_ = arcade::MaxInt(0, width_ - right_x_);
}

bool TetrisGame::Tick(int elapsed_ms) {
  UpdateEffects(elapsed_ms);
  if (paused_ || game_over_) return true;
  if (clearing_) {
    clear_elapsed_ms_ += elapsed_ms;
    if (clear_elapsed_ms_ >= 96) {
      FinishLineClear();
      return true;
    }
    return true;
  }
  bool changed = AnimateStepDrop(elapsed_ms);
  RepeatHorizontalMove(elapsed_ms);
  drop_accum_ += elapsed_ms;
  while (drop_accum_ >= drop_ms_) {
    drop_accum_ -= drop_ms_;
    Drop();
    changed = true;
    if (clearing_ || game_over_) break;
  }
  return changed;
}

void TetrisGame::KeyDown(TetrisKey key) {
  if (key == TETRIS_KEY_BACK) {
    TogglePause();
    return;
  }
  if (clearing_) return;
  if (paused_ && key == TETRIS_KEY_LEFT) {
    return;
  }
  if (paused_ && key == TETRIS_KEY_RIGHT) {
    return;
  }
  if (paused_ && key == TETRIS_KEY_UP) {
    CycleDifficulty();
    return;
  }
  if (paused_ && key == TETRIS_KEY_DOWN) {
    ToggleSoundMute();
    return;
  }
  if (key == TETRIS_KEY_LEFT) {
    bool already_repeating = left_held_ && horizontal_dir_ == -1;
    left_held_ = true;
    horizontal_dir_ = -1;
    horizontal_accum_ = 0;
    horizontal_wait_ms_ = 210;
    if (already_repeating) {
      Move(-1);
      horizontal_wait_ms_ = 55;
    } else {
      Move(-1);
    }
  }
  if (key == TETRIS_KEY_RIGHT) {
    bool already_repeating = right_held_ && horizontal_dir_ == 1;
    right_held_ = true;
    horizontal_dir_ = 1;
    horizontal_accum_ = 0;
    horizontal_wait_ms_ = 210;
    if (already_repeating) {
      Move(1);
      horizontal_wait_ms_ = 55;
    } else {
      Move(1);
    }
  }
  if (key == TETRIS_KEY_DOWN) {
    StartStepDrop(8);
  }
  if (key == TETRIS_KEY_UP) Rotate();
  if (key == TETRIS_KEY_CONFIRM) {
    if (game_over_) {
      Reset();
    } else if (paused_) {
      SetPaused(false);
    } else {
      Rotate();
    }
  }
}

void TetrisGame::KeyUp(TetrisKey key) {
  if (key == TETRIS_KEY_DOWN) down_held_ = false;
  if (key == TETRIS_KEY_LEFT) {
    left_held_ = false;
    if (horizontal_dir_ == -1) {
      horizontal_dir_ = right_held_ ? 1 : 0;
      horizontal_accum_ = 0;
      horizontal_wait_ms_ = 55;
    }
  }
  if (key == TETRIS_KEY_RIGHT) {
    right_held_ = false;
    if (horizontal_dir_ == 1) {
      horizontal_dir_ = left_held_ ? -1 : 0;
      horizontal_accum_ = 0;
      horizontal_wait_ms_ = 55;
    }
  }
}

void TetrisGame::RefillBag() {
  bag_size_ = kPieceCount;
  for (int i = 0; i < kPieceCount; ++i) bag_[i] = i;
  for (int i = bag_size_ - 1; i > 0; --i) {
    int j = static_cast<int>(arcade::NextRandom(&rng_) %
                             static_cast<uint32_t>(i + 1));
    int tmp = bag_[i];
    bag_[i] = bag_[j];
    bag_[j] = tmp;
  }
}

int TetrisGame::TakeBag() {
  if (bag_size_ == 0) RefillBag();
  return bag_[--bag_size_];
}

void TetrisGame::CopyShape(int type, uint8_t out[4][4]) {
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x) out[y][x] = kShapes[type][y][x];
}

void TetrisGame::ShapeTopLeft(const uint8_t cells[4][4], int* left,
                              int* top) const {
  *left = 4;
  *top = 4;
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      if (!cells[y][x]) continue;
      if (x < *left) *left = x;
      if (y < *top) *top = y;
    }
  }
  if (*left == 4) *left = 0;
  if (*top == 4) *top = 0;
}

void TetrisGame::Spawn() {
  step_drop_cells_ = 0;
  step_drop_px_ = 0;
  step_drop_subpx_ = 0;
  current_.type = next_type_;
  current_.x = 3;
  current_.y = -1;
  CopyShape(current_.type, current_.cells);
  next_type_ = TakeBag();
  if (Collides(current_, 0, 0, current_.cells)) game_over_ = true;
}

bool TetrisGame::Collides(const Piece& p, int dx, int dy,
                          const uint8_t cells[4][4]) {
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      if (!cells[y][x]) continue;
      int nx = p.x + x + dx;
      int ny = p.y + y + dy;
      if (nx < 0 || nx >= kCols || ny >= kRows) return true;
      if (ny >= 0 && grid_[ny][nx] >= 0) return true;
    }
  }
  return false;
}

void TetrisGame::Move(int dx) {
  if (paused_ || game_over_) return;
  if (!Collides(current_, dx, 0, current_.cells)) current_.x += dx;
}

void TetrisGame::RepeatHorizontalMove(int elapsed_ms) {
  if (horizontal_dir_ == 0 || paused_ || game_over_) return;
  horizontal_accum_ += elapsed_ms;
  while (horizontal_dir_ != 0 && horizontal_accum_ >= horizontal_wait_ms_) {
    horizontal_accum_ -= horizontal_wait_ms_;
    horizontal_wait_ms_ = 55;
    Move(horizontal_dir_);
  }
}

void TetrisGame::Rotate() {
  if (paused_ || game_over_ || current_.type == O) return;
  uint8_t rotated[4][4];
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x) rotated[y][x] = current_.cells[3 - x][y];
  int old_left = 0;
  int old_top = 0;
  int new_left = 0;
  int new_top = 0;
  ShapeTopLeft(current_.cells, &old_left, &old_top);
  ShapeTopLeft(rotated, &new_left, &new_top);
  int base_dx = old_left - new_left;
  int base_dy = old_top - new_top;
  const int kicks[] = {0, -1, 1, -2, 2};
  for (int i = 0; i < 5; ++i) {
    if (!Collides(current_, base_dx + kicks[i], base_dy, rotated)) {
      current_.x += base_dx + kicks[i];
      current_.y += base_dy;
      memcpy(current_.cells, rotated, sizeof(rotated));
      QueueSound(kSfxPaddle);
      return;
    }
  }
}

void TetrisGame::SoftDrop() {
  if (paused_ || game_over_) return;
  Drop();
}

void TetrisGame::StartStepDrop(int rows) {
  if (paused_ || game_over_ || clearing_) return;
  if (Collides(current_, 0, 1, current_.cells)) return;
  if (step_drop_cells_ <= 0) {
    step_drop_cells_ = kRows;
    step_drop_px_ = 0;
    step_drop_subpx_ = 0;
    QueueSound(kSfxLaser);
  } else if (step_drop_cells_ < kRows) {
    step_drop_cells_ = kRows;
  }
  drop_accum_ = 0;
  (void)rows;
}

bool TetrisGame::AnimateStepDrop(int elapsed_ms) {
  if (step_drop_cells_ <= 0 || paused_ || game_over_ || clearing_) return false;
  int old_y = current_.y;
  int old_px = step_drop_px_;
  int advance_subpx = arcade::MaxInt(1024, cell_ * elapsed_ms * 256 / 6);
  int max_advance_subpx = arcade::MaxInt(8, cell_ * 2) * 256;
  advance_subpx = arcade::MinInt(advance_subpx, max_advance_subpx);
  step_drop_subpx_ += advance_subpx;
  step_drop_px_ = step_drop_subpx_ / 256;
  while (step_drop_cells_ > 0 && step_drop_px_ >= cell_) {
    if (Collides(current_, 0, 1, current_.cells)) {
      step_drop_cells_ = 0;
      step_drop_px_ = 0;
      step_drop_subpx_ = 0;
      Drop();
      return true;
    }
    current_.y += 1;
    --step_drop_cells_;
    step_drop_subpx_ -= cell_ * 256;
    step_drop_px_ = step_drop_subpx_ / 256;
  }
  if (step_drop_cells_ <= 0) {
    step_drop_px_ = 0;
    step_drop_subpx_ = 0;
  }
  if (step_drop_cells_ > 0 && Collides(current_, 0, 1, current_.cells)) {
    step_drop_cells_ = 0;
    step_drop_px_ = 0;
    step_drop_subpx_ = 0;
    Drop();
  }
  drop_accum_ = 0;
  return current_.y != old_y || step_drop_px_ != old_px;
}

void TetrisGame::Drop() {
  if (paused_ || game_over_ || clearing_) return;
  step_drop_cells_ = 0;
  step_drop_px_ = 0;
  step_drop_subpx_ = 0;
  if (!Collides(current_, 0, 1, current_.cells)) {
    current_.y += 1;
    return;
  }
  Merge();
  StartLineClear();
  if (!clearing_) Spawn();
}

void TetrisGame::Merge() {
  step_drop_cells_ = 0;
  step_drop_px_ = 0;
  step_drop_subpx_ = 0;
  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      if (current_.cells[y][x] && current_.y + y >= 0) {
        grid_[current_.y + y][current_.x + x] = current_.type;
        SpawnCellEffect(current_.x + x, current_.y + y, current_.type, 3);
      }
    }
  }
  QueueSound(kSfxBrick);
}

void TetrisGame::StartLineClear() {
  clear_count_ = 0;
  for (int y = kRows - 1; y >= 0; --y) {
    bool full = true;
    for (int x = 0; x < kCols; ++x) {
      if (grid_[y][x] < 0) {
        full = false;
        break;
      }
    }
    if (!full) continue;
    clear_rows_[clear_count_++] = y;
  }
  if (clear_count_ == 0) return;
  clearing_ = true;
  clear_elapsed_ms_ = 0;
  QueueSound(kSfxBonus);
}

void TetrisGame::SpawnCellEffect(int gx, int gy, int type, int count) {
  float x = static_cast<float>(board_x_ + board_border_ + gx * cell_ +
                               cell_ / 2);
  float y = static_cast<float>(board_y_ + board_border_ + gy * cell_ +
                               cell_ / 2);
  pixel_effects::SpawnParticles(particles_, kMaxParticles, x, y, type, count,
                                &rng_);
}

void TetrisGame::SpawnLineEffect(int row) {
  int play_x = board_x_ + board_border_;
  int play_y = board_y_ + board_border_;
  float y = static_cast<float>(play_y + row * cell_ + cell_ / 2);
  for (int x = 0; x < kCols; ++x) {
    int type = grid_[row][x] >= 0 ? grid_[row][x] : x % kPieceCount;
    float px = static_cast<float>(play_x + x * cell_ + cell_ / 2);
    pixel_effects::SpawnParticles(particles_, kMaxParticles, px, y, type, 5,
                                  &rng_);
    if ((x & 1) == 0) {
      pixel_effects::SpawnBurst(bursts_, kMaxBursts, px, y, type);
    }
  }
}

void TetrisGame::UpdateEffects(double dt_ms) {
  pixel_effects::UpdateParticles(particles_, kMaxParticles, dt_ms);
  pixel_effects::UpdateBursts(bursts_, kMaxBursts, dt_ms);
}

void TetrisGame::FinishLineClear() {
  int cleared = clear_count_;
  for (int i = 0; i < clear_count_; ++i) SpawnLineEffect(clear_rows_[i]);
  for (int i = 0; i < clear_count_; ++i) {
    int y = clear_rows_[i];
    for (int yy = y; yy > 0; --yy)
      for (int x = 0; x < kCols; ++x) grid_[yy][x] = grid_[yy - 1][x];
    for (int x = 0; x < kCols; ++x) grid_[0][x] = -1;
    for (int j = i + 1; j < clear_count_; ++j)
      if (clear_rows_[j] < y) ++clear_rows_[j];
  }
  int old_bucket = lines_ / 5;
  lines_ += cleared;
  static const int scores[] = {0, 10, 25, 45, 80};
  score_ += scores[cleared < 4 ? cleared : 4];
  int new_bucket = lines_ / 5;
  if (new_bucket != old_bucket)
    pattern_index_ = new_bucket % TetrisPatternVariantCount();
  if (new_bucket != old_bucket) QueueSound(kSfxLevel);
  RecomputeDropSpeed();
  clear_count_ = 0;
  clear_elapsed_ms_ = 0;
  clearing_ = false;
  Spawn();
}

void TetrisGame::RecomputeDropSpeed() {
  drop_ms_ = arcade::MaxInt(60, player_drop_ms_ - (lines_ / 10) * 32);
}

void TetrisGame::AdjustPausedSpeed(int delta_ms) {
  player_drop_ms_ += delta_ms;
  if (player_drop_ms_ < 120) player_drop_ms_ = 120;
  if (player_drop_ms_ > 760) player_drop_ms_ = 760;
  drop_accum_ = 0;
  RecomputeDropSpeed();
}

void TetrisGame::CycleDifficulty() {
  int speed_level = 1 + (760 - player_drop_ms_) / 80;
  if (speed_level < 1) speed_level = 1;
  if (speed_level > 9) speed_level = 9;
  speed_level = arcade::CycleInt(speed_level, 1, 9);
  player_drop_ms_ = 760 - (speed_level - 1) * 80;
  RecomputeDropSpeed();
  drop_accum_ = 0;
  QueueSound(kSfxPause);
}

void TetrisGame::SetPaused(bool paused) {
  if (game_over_) return;
  if (paused_ == paused) return;
  paused_ = paused;
  if (!paused_) drop_accum_ = 0;
  QueueSound(kSfxPause);
}

void TetrisGame::TogglePause() {
  if (game_over_) {
    Reset();
    return;
  }
  SetPaused(!paused_);
}

void TetrisGame::ToggleSoundMute() {
  if (!paused_ || game_over_) return;
  sound_muted_ = !sound_muted_;
  QueueSound(kSfxPause);
}

void TetrisGame::QueueSound(ArcadeSfx sfx) {
  if (sound_muted_) return;
  arcade::QueueSoundEvent(&sound_events_, sfx);
}

uint32_t TetrisGame::ConsumeSoundEvents() {
  return arcade::ConsumeSoundEvents(&sound_events_);
}

void TetrisGame::FillRect(int x, int y, int w, int h, TetrisColor c) {
  if (!renderer_ || w <= 0 || h <= 0) return;
  renderer_->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(w), static_cast<float>(h)},
                      ToGles(c));
}

void TetrisGame::OutlineRect(int x, int y, int w, int h, TetrisColor c, int t) {
  if (!renderer_ || w <= 0 || h <= 0 || t <= 0) return;
  GlesColor color = ToGles(c);
  renderer_->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(w), static_cast<float>(t)},
                      color);
  renderer_->FillRect(GlesRect{static_cast<float>(x),
                               static_cast<float>(y + h - t),
                               static_cast<float>(w), static_cast<float>(t)},
                      color);
  renderer_->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(t), static_cast<float>(h)},
                      color);
  renderer_->FillRect(GlesRect{static_cast<float>(x + w - t),
                               static_cast<float>(y), static_cast<float>(t),
                               static_cast<float>(h)},
                      color);
}

void TetrisGame::DrawBlockPixels(int x, int y, int size, int type) {
  const arcade::Palette& p = arcade::DefaultPalette();
  const GlesColor base = p.pieces[type];
  const AsciiSpritePalette sprite = MakeAsciiSpritePalette(
      arcade::Darken(base), arcade::Darken(base), base, arcade::Lighten(base), p.block_highlight, base,
      base);
  const float block_inset = 0.5f;
  DrawAsciiSprite(
      renderer_, stackfall_assets::kBlock,
      GlesRect{static_cast<float>(x) + block_inset,
               static_cast<float>(y) + block_inset,
               static_cast<float>(size) - block_inset * 2.0f,
               static_cast<float>(size) - block_inset * 2.0f},
      sprite);
}

void TetrisGame::DrawBlock(int gx, int gy, int type) {
  int x = board_x_ + board_border_ + gx * cell_;
  int y = board_y_ + board_border_ + gy * cell_;
  DrawBlockPixels(x, y, cell_, type);
}

void TetrisGame::DrawBoard() {
  const arcade::Palette& p = arcade::DefaultPalette();
  FillRect(board_x_, board_y_, board_w_, board_h_, {5, 9, 20});
  int play_x = board_x_ + board_border_;
  int play_y = board_y_ + board_border_;
  int play_w = cell_ * kCols;
  int play_h = cell_ * kRows;
  renderer_->FillRect(
      GlesRect{static_cast<float>(play_x), static_cast<float>(play_y),
               static_cast<float>(play_w), static_cast<float>(play_h)},
      p.playfield_bg);
  renderer_->SetBlendMode(kGlesBlendAlpha);
  GlesColor grid = p.grid;
  grid.a = 0.45f;
  for (int x = 1; x < kCols; ++x)
    renderer_->FillRect(
        GlesRect{static_cast<float>(play_x + x * cell_),
                 static_cast<float>(play_y), 1.0f,
                 static_cast<float>(play_h)},
        grid);
  for (int y = 1; y < kRows; ++y)
    renderer_->FillRect(
        GlesRect{static_cast<float>(play_x),
                 static_cast<float>(play_y + y * cell_),
                 static_cast<float>(play_w), 1.0f},
        grid);
  renderer_->SetBlendMode(kGlesBlendOpaque);
  bool blink_on = !clearing_ || ((clear_elapsed_ms_ / 16) % 2 == 0);
  for (int y = 0; y < kRows; ++y) {
    bool clearing_row = IsClearingRow(y);
    for (int x = 0; x < kCols; ++x) {
      if (grid_[y][x] >= 0 && (!clearing_row || blink_on)) {
        DrawBlock(x, y, grid_[y][x]);
      }
    }
  }
  if (!clearing_) {
    for (int y = 0; y < 4; ++y) {
      for (int x = 0; x < 4; ++x) {
        int gy = current_.y + y;
        if (current_.cells[y][x] && gy >= 0) {
          int px = play_x + (current_.x + x) * cell_;
          int py = play_y + gy * cell_ + step_drop_px_;
          DrawBlockPixels(px, py, cell_, current_.type);
        }
      }
    }
  }
  arcade::DrawBorder(
      renderer_,
      GlesRect{static_cast<float>(board_x_), static_cast<float>(board_y_),
               static_cast<float>(board_w_), static_cast<float>(board_h_)},
      p.board_border, static_cast<float>(board_border_));
}

void TetrisGame::DrawEffects() {
  const arcade::Palette& p = arcade::DefaultPalette();
  renderer_->SetBlendMode(kGlesBlendAlpha);
  pixel_effects::DrawBursts(renderer_, bursts_, kMaxBursts, p.pieces, 7,
                            62.0f,
                            static_cast<float>(arcade::MaxInt(3, board_w_ / 160)));
  pixel_effects::DrawParticles(renderer_, particles_, kMaxParticles, p.pieces,
                               7);
  renderer_->SetBlendMode(kGlesBlendOpaque);
}

bool TetrisGame::IsClearingRow(int row) const {
  if (!clearing_) return false;
  for (int i = 0; i < clear_count_; ++i)
    if (clear_rows_[i] == row) return true;
  return false;
}

void TetrisGame::DrawDigit(int x, int y, int s, int value, TetrisColor on) {
  static const uint8_t map[10] = {
      0x77, 0x12, 0x5d, 0x5b, 0x3a, 0x6b, 0x6f, 0x52, 0x7f, 0x7b};
  uint8_t m = map[value % 10];
  TetrisColor off = {38, 34, 53};
  int t = arcade::MaxInt(4, s / 5);
  int w = s;
  int h = s * 2;
  FillRect(x + t, y, w - 2 * t, t, (m & 0x40) ? on : off);
  FillRect(x, y + t, t, h / 2 - t, (m & 0x20) ? on : off);
  FillRect(x + w - t, y + t, t, h / 2 - t, (m & 0x10) ? on : off);
  FillRect(x + t, y + h / 2, w - 2 * t, t, (m & 0x08) ? on : off);
  FillRect(x, y + h / 2 + t, t, h / 2 - t, (m & 0x04) ? on : off);
  FillRect(x + w - t, y + h / 2 + t, t, h / 2 - t,
           (m & 0x02) ? on : off);
  FillRect(x + t, y + h, w - 2 * t, t, (m & 0x01) ? on : off);
}

void TetrisGame::DrawNumber(int x, int y, int s, int value, int digits,
                            TetrisColor on) {
  int divisor = 1;
  for (int i = 1; i < digits; ++i) divisor *= 10;
  for (int i = 0; i < digits; ++i) {
    DrawDigit(x + i * (s + 8), y, s, (value / divisor) % 10, on);
    divisor /= 10;
  }
}

void TetrisGame::DrawMiniPiece(int x, int y, int box) {
  int b = cell_;
  if (b * 4 > box) b = box / 4;
  int ox = x + (box - 4 * b) / 2;
  int oy = y + (box - 4 * b) / 2;
  for (int yy = 0; yy < 4; ++yy)
    for (int xx = 0; xx < 4; ++xx)
      if (kShapes[next_type_][yy][xx]) {
        DrawBlockPixels(ox + xx * b, oy + yy * b, b, next_type_);
      }
}

void TetrisGame::HudRect(int* hx, int* hy, int* hw, int* hh) const {
  int gap = arcade::SideHudGap(width_);
  int total_w = arcade::MinInt(arcade::SideHudWidth(width_),
                       arcade::MaxInt(0, right_w_ - gap - arcade::ScreenMargin(width_, height_)));
  *hx = right_x_ + gap;
  *hy = board_y_;
  *hw = total_w;
  *hh = board_h_;
}

void TetrisGame::DrawHud() {
  int x = 0;
  int y = 0;
  int total_w = 0;
  int panel_h = 0;
  HudRect(&x, &y, &total_w, &panel_h);
  if (total_w <= 0 || panel_h <= 0) return;
  arcade::HudLayout layout = arcade::ComputeHudLayout(
      GlesRect{static_cast<float>(x), static_cast<float>(y),
               static_cast<float>(total_w), static_cast<float>(panel_h)}, 4);
  int label_scale = layout.label_scale;
  int content_x = layout.content_x;
  int content_w = layout.content_w;
  arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::DrawHudPanel(renderer_, layout, style);
  arcade::HudFlow flow = arcade::BeginHudFlow(layout);

  int label_gap = label_scale * 17;
  int box = arcade::MaxInt(12, arcade::MinInt(content_w, cell_ * 4));
  int speed_level = 1 + (760 - player_drop_ms_) / 80;
  if (speed_level < 1) speed_level = 1;
  if (speed_level > 9) speed_level = 9;

  arcade::DrawHudNumber(renderer_, &flow, layout, "SCORE", score_, 4,
                        style.value, style);

  arcade::DrawHudNumber(renderer_, &flow, layout, "SPEED", speed_level, 1,
                        style.value_alt, style);

  arcade::DrawHudTextValue(renderer_, &flow, layout, "SOUND",
                           sound_muted_ ? "OFF" : "ON",
                           sound_muted_ ? arcade::DefaultPalette().game_over_border
                                        : style.value,
                           style);

  int next_y = arcade::ReserveHudBlock(&flow, label_gap + box);
  arcade::DrawPixelText(renderer_, "NEXT", content_x, next_y, label_scale,
                        style.label);
  DrawMiniPiece(content_x + (content_w - box) / 2,
                next_y + label_gap, box);
}

void TetrisGame::DrawPatternPanel(int x, int y, int w, int h, int lines) {
  if (w <= 0 || h <= 0) return;
  TetrisPattern pattern = TetrisPatternForLines(lines);
  FillRect(x, y, w, h, pattern.background);
  int step = arcade::MaxInt(92, cell_ * 2 + pattern.step_extra);
  for (int local_y = -step; local_y < h + step; local_y += step) {
    for (int local_x = -step; local_x < w + step; local_x += step) {
      int xx = x + local_x;
      int yy = y + local_y;
      int gx = local_x / step;
      int gy = local_y / step;
      int selector = (gx * 3 + gy * 5 + pattern.phase) & 3;
      switch (pattern.motif) {
        case 0:
          if (selector == 0) {
            FillRect(xx + step / 5, yy + step / 5, step * 3 / 5,
                     step * 3 / 5, pattern.primary);
          } else if (selector == 1) {
            FillRect(xx, yy + step / 3, step, arcade::MaxInt(8, step / 7),
                     pattern.secondary);
            FillRect(xx + step / 3, yy, arcade::MaxInt(8, step / 7), step,
                     pattern.secondary);
          } else if (selector == 2) {
            OutlineRect(xx + 5, yy + 5, step - 10, step - 10, pattern.accent,
                        arcade::MaxInt(5, step / 9));
          } else {
            FillRect(xx + step / 7, yy + step / 7, step / 5, step / 5,
                     pattern.primary);
            FillRect(xx + step * 4 / 7, yy + step * 4 / 7, step / 5,
                     step / 5, pattern.secondary);
          }
          break;
        case 1:
          FillRect(xx + step / 10, yy + step * 6 / 10, step * 7 / 10,
                   arcade::MaxInt(8, step / 8), pattern.secondary);
          FillRect(xx + step * 3 / 10, yy + step * 4 / 10, step * 7 / 10,
                   arcade::MaxInt(8, step / 8), pattern.primary);
          FillRect(xx + step * 5 / 10, yy + step * 2 / 10, step * 5 / 10,
                   arcade::MaxInt(8, step / 8), pattern.accent);
          break;
        case 2:
          OutlineRect(xx + step / 8, yy + step / 8, step * 3 / 4,
                      step * 3 / 4, pattern.primary, arcade::MaxInt(5, step / 12));
          OutlineRect(xx + step / 3, yy + step / 3, step / 3, step / 3,
                      pattern.secondary, arcade::MaxInt(4, step / 16));
          if (selector == 0)
            FillRect(xx + step * 7 / 10, yy + step / 10, step / 8, step / 8,
                     pattern.accent);
          break;
        case 3:
          FillRect(xx + step / 6, yy + step / 6, step / 3, step / 3,
                   pattern.primary);
          FillRect(xx + step / 2, yy + step / 2, step / 3, step / 3,
                   pattern.secondary);
          FillRect(xx + step * 7 / 20, yy, arcade::MaxInt(7, step / 10), step,
                   pattern.accent);
          FillRect(xx, yy + step * 7 / 20, step, arcade::MaxInt(7, step / 10),
                   pattern.accent);
          break;
        case 4:
          FillRect(xx + step / 8, yy + step / 6, step * 3 / 4,
                   arcade::MaxInt(10, step / 7), pattern.primary);
          FillRect(xx + step / 8, yy + step / 2, step * 3 / 4,
                   arcade::MaxInt(10, step / 7), pattern.secondary);
          FillRect(xx + step / 3, yy + step / 6, arcade::MaxInt(10, step / 7),
                   step / 2, pattern.accent);
          break;
        case 5:
          FillRect(xx + step / 10, yy + step / 10, step / 6, step / 6,
                   pattern.accent);
          FillRect(xx + step * 4 / 10, yy + step * 2 / 10, step / 4,
                   step / 4, pattern.primary);
          FillRect(xx + step * 7 / 10, yy + step * 6 / 10, step / 5,
                   step / 5, pattern.secondary);
          if (selector & 1)
            FillRect(xx + step / 8, yy + step * 7 / 10, step / 4, step / 7,
                     pattern.secondary);
          break;
        case 6:
          FillRect(xx + step / 7, yy + step / 7, step * 5 / 7, step * 5 / 7,
                   pattern.primary);
          FillRect(xx + step / 4, yy + step / 4, step / 2, step / 2,
                   pattern.background);
          OutlineRect(xx + step / 3, yy + step / 3, step / 3, step / 3,
                      pattern.accent, arcade::MaxInt(4, step / 18));
          break;
        default:
          FillRect(xx + step / 8, yy + step / 5, step / 3, arcade::MaxInt(9, step / 8),
                   pattern.primary);
          FillRect(xx + step * 3 / 8, yy + step * 2 / 5, step / 3,
                   arcade::MaxInt(9, step / 8), pattern.secondary);
          FillRect(xx + step * 5 / 8, yy + step * 3 / 5, step / 3,
                   arcade::MaxInt(9, step / 8), pattern.accent);
          FillRect(xx + step / 8, yy + step * 4 / 5, step * 5 / 6,
                   arcade::MaxInt(7, step / 12), pattern.primary);
          break;
      }
    }
  }
}

void TetrisGame::DrawPatterns() {
  DrawPatternPanel(0, 0, board_x_, height_, lines_);
  DrawPatternPanel(right_x_, 0, width_ - right_x_, height_, lines_);
}

void TetrisGame::DrawOverlay() {
  if (!paused_ && !game_over_) return;
  const char* text = game_over_ ? "GAME OVER" : "PAUSED";
  int scale = arcade::MaxInt(3, cell_ / 5);
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawCenteredOverlay(
      renderer_,
      GlesRect{static_cast<float>(board_x_), static_cast<float>(board_y_),
               static_cast<float>(board_w_), static_cast<float>(board_h_)},
      text, game_over_ ? p.game_over_border : p.pause_border, scale);
}

void TetrisGame::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  renderer_ = renderer;
  renderer_->BeginFrame(arcade::DefaultPalette().screen_bg);
  FillRect(0, 0, width_, height_, {5, 7, 18});
  DrawPatterns();
  DrawHud();
  DrawBoard();
  DrawEffects();
  DrawOverlay();
  renderer_ = 0;
}
