#include "lightline_game.h"
#include "lightline_assets.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/color_util.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"

#include <string.h>

TronGame::TronGame()
    : width_(1280),
      height_(720),
      arena_x_(0),
      arena_y_(0),
      arena_w_(0),
      arena_h_(0),
      cell_(0),
      border_(0),
      hud_x_(0),
      hud_y_(0),
      hud_w_(0),
      hud_h_(0),
      score_(0),
      rounds_(0),
      speed_(5),
      step_ms_(92.0),
      accumulator_ms_(0.0),
      paused_(false),
      game_over_(false),
      player_won_(false),
      sound_muted_(false),
      sound_events_(0),
      rng_(0x7700120u),
      opponent_count_(1) {
  memset(grid_, 0, sizeof(grid_));
  memset(opponents_, 0, sizeof(opponents_));
  memset(bursts_, 0, sizeof(bursts_));
  Reset();
}

void TronGame::Reset() {
  ClearGrid();
  accumulator_ms_ = 0.0;
  paused_ = false;
  game_over_ = false;
  player_won_ = false;
  sound_events_ = 0;
  player_ = Bike{kCols / 4, kRows / 2, 1, 0, 0, true};
  grid_[player_.y][player_.x] = 1;
  static const Bike kStarts[kMaxOpponents] = {
      {kCols * 3 / 4, kRows / 2, -1, 0, 5, true},
      {kCols / 2, kRows / 4, 0, 1, 4, true},
      {kCols / 2, kRows * 3 / 4, 0, -1, 3, true},
      {kCols * 3 / 4, kRows / 4, -1, 0, 2, true},
      {kCols * 3 / 4, kRows * 3 / 4, -1, 0, 1, true}};
  opponent_count_ = OpponentCountForLevel();
  memset(opponents_, 0, sizeof(opponents_));
  for (int i = 0; i < opponent_count_; ++i) {
    opponents_[i] = kStarts[i];
    grid_[opponents_[i].y][opponents_[i].x] =
        static_cast<uint8_t>(opponents_[i].color + 1);
  }
  memset(bursts_, 0, sizeof(bursts_));
}

int TronGame::OpponentCountForLevel() const {
  const int level = rounds_ + 1;
  // Levels 1-2 have one rival; levels 3-5 have two, and so on.
  return arcade::MinInt(kMaxOpponents, 1 + level / 3);
}

void TronGame::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  Layout();
}

void TronGame::Update(double dt_ms, const InputState& input) {
  pixel_effects::UpdateBursts(bursts_, kMaxBursts, dt_ms);
  if (game_over_) {
    if (input.Pressed(kActionPrimary)) {
      ++rounds_;
      Reset();
    }
    return;
  }
  if (paused_) return;
  SteerPlayer(input);
  accumulator_ms_ += dt_ms;
  const double max_acc = step_ms_ * 3.0;
  if (accumulator_ms_ > max_acc) accumulator_ms_ = max_acc;
  while (accumulator_ms_ >= step_ms_ && !game_over_) {
    Step();
    accumulator_ms_ -= step_ms_;
  }
}

void TronGame::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  DrawBackground(renderer);
  DrawArena(renderer);
  DrawHud(renderer);
  pixel_effects::DrawBursts(renderer, bursts_, kMaxBursts,
                            arcade::DefaultPalette().pieces, 7,
                            static_cast<float>(cell_ * 3), 4.0f);
  if (paused_ || game_over_) DrawOverlay(renderer);
}

void TronGame::SetPaused(bool paused) {
  if (game_over_ || paused_ == paused) return;
  paused_ = paused;
  QueueSound(kSfxPause);
}

void TronGame::ToggleSoundMute() { sound_muted_ = !sound_muted_; }

void TronGame::CycleDifficulty() {
  speed_ = arcade::CycleInt(speed_, 1, 9);
  step_ms_ = 150.0 - speed_ * 12.0;
  QueueSound(kSfxLevel);
}

uint32_t TronGame::ConsumeSoundEvents() {
  return arcade::ConsumeSoundEvents(&sound_events_);
}

void TronGame::Layout() {
  const arcade::GridSideLayout layout = arcade::ComputeGridSideLayout(
      width_, height_, kCols, kRows, 8, true);
  cell_ = layout.cell;
  arena_x_ = layout.play_x; arena_y_ = layout.play_y;
  arena_w_ = layout.play_w; arena_h_ = layout.play_h;
  border_ = layout.border;
  hud_x_ = layout.hud_x; hud_y_ = layout.hud_y;
  hud_w_ = layout.hud_w; hud_h_ = layout.hud_h;
}

void TronGame::ClearGrid() { memset(grid_, 0, sizeof(grid_)); }

void TronGame::Step() {
  for (int i = 0; i < opponent_count_; ++i)
    if (opponents_[i].alive) SteerAi(i);

  Bike* bikes[kMaxOpponents + 1];
  bikes[0] = &player_;
  for (int i = 0; i < opponent_count_; ++i) bikes[i + 1] = &opponents_[i];
  const int bike_count = opponent_count_ + 1;
  bool crashed[kMaxOpponents + 1] = {};
  int next_x[kMaxOpponents + 1] = {};
  int next_y[kMaxOpponents + 1] = {};
  for (int i = 0; i < bike_count; ++i) {
    if (!bikes[i]->alive) continue;
    next_x[i] = bikes[i]->x + bikes[i]->dx;
    next_y[i] = bikes[i]->y + bikes[i]->dy;
    crashed[i] = !CanMove(next_x[i], next_y[i]);
  }
  for (int i = 0; i < bike_count; ++i) {
    if (!bikes[i]->alive) continue;
    for (int j = i + 1; j < bike_count; ++j) {
      if (!bikes[j]->alive) continue;
      if (next_x[i] == next_x[j] && next_y[i] == next_y[j])
        crashed[i] = crashed[j] = true;
    }
  }

  for (int i = 0; i < bike_count; ++i) {
    if (!bikes[i]->alive || crashed[i]) continue;
    bikes[i]->x = next_x[i];
    bikes[i]->y = next_y[i];
  }

  for (int i = 0; i < bike_count; ++i) {
    if (!bikes[i]->alive || !crashed[i]) continue;
    bikes[i]->alive = false;
    const int burst_color = i == 0 ? 6 : opponents_[i - 1].color;
    pixel_effects::SpawnBurst(bursts_, kMaxBursts,
                              arena_x_ + bikes[i]->x * cell_ + cell_ / 2,
                              arena_y_ + bikes[i]->y * cell_ + cell_ / 2,
                              burst_color);
    if (i > 0) score_ += 25 * speed_;
  }

  int alive_opponents = 0;
  for (int i = 0; i < opponent_count_; ++i)
    if (opponents_[i].alive) ++alive_opponents;
  if (!player_.alive || alive_opponents == 0) {
    game_over_ = true;
    player_won_ = player_.alive && alive_opponents == 0;
    if (player_won_) score_ += 100 * speed_;
    QueueSound(player_won_ ? kSfxLevel : kSfxLose);
    return;
  }

  if (player_.alive) grid_[player_.y][player_.x] = 1;
  for (int i = 0; i < opponent_count_; ++i)
    if (opponents_[i].alive)
      grid_[opponents_[i].y][opponents_[i].x] =
          static_cast<uint8_t>(opponents_[i].color + 1);
  score_ += 1;
  QueueSound(kSfxBrick);
}

void TronGame::SteerPlayer(const InputState& input) {
  if (input.Pressed(kActionLeft) && player_.dx == 0) {
    player_.dx = -1; player_.dy = 0;
  } else if (input.Pressed(kActionRight) && player_.dx == 0) {
    player_.dx = 1; player_.dy = 0;
  } else if (input.Pressed(kActionUp) && player_.dy == 0) {
    player_.dx = 0; player_.dy = -1;
  } else if (input.Pressed(kActionDown) && player_.dy == 0) {
    player_.dx = 0; player_.dy = 1;
  }
}

void TronGame::SteerAi(int index) {
  Bike& ai = opponents_[index];
  int dirs[4][2] = {{ai.dx, ai.dy}, {ai.dy, -ai.dx},
                    {-ai.dy, ai.dx}, {-ai.dx, -ai.dy}};
  int best = 0;
  int best_score = -100000;
  for (int i = 0; i < 4; ++i) {
    if (dirs[i][0] == -ai.dx && dirs[i][1] == -ai.dy) continue;
    int nx = ai.x + dirs[i][0];
    int ny = ai.y + dirs[i][1];
    if (!CanMove(nx, ny)) continue;

    int straight = 0;
    int sx = nx;
    int sy = ny;
    while (CanMove(sx, sy) && straight < 16) {
      ++straight;
      sx += dirs[i][0];
      sy += dirs[i][1];
    }

    int space = CountReachableCells(nx, ny);
    int edge = arcade::MinInt(arcade::MinInt(nx, kCols - 1 - nx), arcade::MinInt(ny, kRows - 1 - ny));
    int player_distance = arcade::MaxInt(player_.x - nx, nx - player_.x) +
                          arcade::MaxInt(player_.y - ny, ny - player_.y);
    const int lookahead = 5;
    const int target_x = player_.x + player_.dx * lookahead;
    const int target_y = player_.y + player_.dy * lookahead;
    int intercept_distance = arcade::MaxInt(target_x - nx, nx - target_x) +
                             arcade::MaxInt(target_y - ny, ny - target_y);
    int turn_penalty = (dirs[i][0] == ai.dx && dirs[i][1] == ai.dy) ? 0 : 5;
    int intercept_bonus = (nx == target_x || ny == target_y) ? 10 : 0;
    int score = space * 5 + straight * 8 + edge * 3 - player_distance * 2 -
                intercept_distance * 2 + intercept_bonus - turn_penalty +
                static_cast<int>(arcade::NextRandom(&rng_) % 4);

    if (score > best_score) {
      best_score = score;
      best = i;
    }
  }
  ai.dx = dirs[best][0];
  ai.dy = dirs[best][1];
}

int TronGame::CountReachableCells(int start_x, int start_y) const {
  if (!CanMove(start_x, start_y)) return 0;
  bool seen[kRows][kCols];
  int qx[kRows * kCols];
  int qy[kRows * kCols];
  memset(seen, 0, sizeof(seen));
  int head = 0;
  int tail = 0;
  qx[tail] = start_x;
  qy[tail] = start_y;
  ++tail;
  seen[start_y][start_x] = true;
  int count = 0;
  while (head < tail && count < 96) {
    int x = qx[head];
    int y = qy[head];
    ++head;
    ++count;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (int i = 0; i < 4; ++i) {
      int nx = x + dirs[i][0];
      int ny = y + dirs[i][1];
      if (nx < 0 || nx >= kCols || ny < 0 || ny >= kRows) continue;
      if (seen[ny][nx] || grid_[ny][nx] != 0) continue;
      seen[ny][nx] = true;
      qx[tail] = nx;
      qy[tail] = ny;
      ++tail;
    }
  }
  return count;
}

bool TronGame::CanMove(int x, int y) const {
  return x >= 0 && x < kCols && y >= 0 && y < kRows && grid_[y][x] == 0;
}

void TronGame::QueueSound(ArcadeSfx sfx) {
  arcade::QueueSoundEvent(&sound_events_, sfx);
}

void TronGame::DrawBackground(Gles2Renderer* renderer) {
  renderer->BeginFrame(arcade::DefaultPalette().screen_bg);
  arcade::DrawBackgroundPattern(
      renderer,
      GlesRect{0, 0, static_cast<float>(width_), static_cast<float>(height_)},
      rounds_ * 5 + speed_ * 2 + score_ / 80, arcade::MaxInt(58, width_ / 18));
}

void TronGame::DrawArena(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  const GlesRect frame = {
      static_cast<float>(arena_x_ - border_),
      static_cast<float>(arena_y_ - border_),
      static_cast<float>(arena_w_ + border_ * 2),
      static_cast<float>(arena_h_ + border_ * 2)};
  arcade::DrawPanel(renderer, frame, p.playfield_bg, p.board_border,
                    static_cast<float>(border_));
  renderer->SetBlendMode(kGlesBlendAlpha);
  GlesColor grid = p.grid; grid.a = 0.55f;
  for (int x = 1; x < kCols; ++x)
    renderer->FillRect(GlesRect{static_cast<float>(arena_x_ + x * cell_),
                                static_cast<float>(arena_y_), 1.0f,
                                static_cast<float>(arena_h_)}, grid);
  for (int y = 1; y < kRows; ++y)
    renderer->FillRect(GlesRect{static_cast<float>(arena_x_),
                                static_cast<float>(arena_y_ + y * cell_),
                                static_cast<float>(arena_w_), 1.0f}, grid);
  renderer->SetBlendMode(kGlesBlendOpaque);
  const arcade::Palette& pal = arcade::DefaultPalette();
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (!grid_[y][x]) continue;
      GlesColor c = pal.pieces[(grid_[y][x] - 1) % 6];
      const AsciiSpritePalette trail = MakeAsciiSpritePalette(
          arcade::Darken(c), arcade::Darken(c), c, arcade::Lighten(c), p.block_highlight, c, c);
      DrawAsciiSprite(renderer, lightline_assets::kBall,
          GlesRect{static_cast<float>(arena_x_ + x * cell_),
                   static_cast<float>(arena_y_ + y * cell_),
                   static_cast<float>(cell_), static_cast<float>(cell_)}, trail);
    }
  }
  const AsciiSpritePalette player_sprite = MakeAsciiSpritePalette(
      arcade::Darken(pal.pieces[0]), arcade::Darken(pal.pieces[0]), pal.pieces[0],
      arcade::Lighten(pal.pieces[0]), p.block_highlight, pal.pieces[0], pal.pieces[0]);
  DrawAsciiSprite(renderer, lightline_assets::kBall,
      GlesRect{static_cast<float>(arena_x_ + player_.x * cell_),
               static_cast<float>(arena_y_ + player_.y * cell_),
               static_cast<float>(cell_), static_cast<float>(cell_)}, player_sprite);
  for (int i = 0; i < opponent_count_; ++i) {
    const Bike& ai = opponents_[i];
    if (!ai.alive) continue;
    const GlesColor color = pal.pieces[ai.color];
    const AsciiSpritePalette ai_sprite = MakeAsciiSpritePalette(
        arcade::Darken(color), arcade::Darken(color), color, arcade::Lighten(color),
        p.block_highlight, color, color);
    DrawAsciiSprite(renderer, lightline_assets::kBall,
        GlesRect{static_cast<float>(arena_x_ + ai.x * cell_),
                 static_cast<float>(arena_y_ + ai.y * cell_),
                 static_cast<float>(cell_), static_cast<float>(cell_)}, ai_sprite);
  }
  arcade::DrawBorder(renderer, frame, p.board_border,
                     static_cast<float>(border_));
}

void TronGame::DrawHud(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::HudLayout layout = arcade::ComputeHudLayout(
      GlesRect{static_cast<float>(hud_x_), static_cast<float>(hud_y_),
               static_cast<float>(hud_w_), static_cast<float>(hud_h_)}, 5);
  arcade::DrawHudPanel(renderer, layout, style);
  arcade::HudFlow flow = arcade::BeginHudFlow(layout);
  arcade::DrawHudNumber(renderer, &flow, layout, "SCORE", score_, 5,
                        p.score, style);
  arcade::DrawHudNumber(renderer, &flow, layout, "LEVEL", rounds_ + 1, 2,
                        p.speed, style);
  arcade::DrawHudNumber(renderer, &flow, layout, "RIVALS", opponent_count_, 1,
                        p.game_over_border, style);
  arcade::DrawHudNumber(renderer, &flow, layout, "SPEED", speed_, 1,
                        p.speed, style);
  arcade::DrawHudTextValue(renderer, &flow, layout, "SOUND",
                           sound_muted_ ? "OFF" : "ON",
                           sound_muted_ ? p.game_over_border : p.score, style);
}

void TronGame::DrawOverlay(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawCenteredOverlay(
      renderer, GlesRect{static_cast<float>(arena_x_),
                         static_cast<float>(arena_y_),
                         static_cast<float>(arena_w_),
                         static_cast<float>(arena_h_)},
      game_over_ ? (player_won_ ? "YOU WIN" : "CRASH") : "PAUSED",
      game_over_ ? (player_won_ ? p.board_border : p.game_over_border)
                 : p.pause_border,
      arcade::MaxInt(3, arena_w_ / 220));
}
