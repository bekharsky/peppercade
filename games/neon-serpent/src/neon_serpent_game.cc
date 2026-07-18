#include "neon_serpent_game.h"
#include "neon_serpent_assets.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/color_util.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"

#include <string.h>

SnakeGame::SnakeGame()
    : width_(1280),
      height_(720),
      play_x_(0),
      play_y_(0),
      play_w_(0),
      play_h_(0),
      cell_(0),
      border_(0),
      hud_x_(0),
      hud_y_(0),
      hud_w_(0),
      hud_h_(0),
      dir_x_(1),
      dir_y_(0),
      next_dir_x_(1),
      next_dir_y_(0),
      length_(4),
      score_(0),
      speed_(4),
      step_ms_(150.0),
      accumulator_ms_(0.0),
      paused_(false),
      game_over_(false),
      sound_muted_(false),
      sound_events_(0),
      rng_(0x51a1e5u) {
  memset(snake_, 0, sizeof(snake_));
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  Reset();
}

void SnakeGame::Reset() {
  score_ = 0;
  accumulator_ms_ = 0.0;
  paused_ = false;
  game_over_ = false;
  sound_events_ = 0;
  dir_x_ = next_dir_x_ = 1;
  dir_y_ = next_dir_y_ = 0;
  length_ = 4;
  int sx = kCols / 2 - 2;
  int sy = kRows / 2;
  for (int i = 0; i < length_; ++i) {
    snake_[i].x = sx - i;
    snake_[i].y = sy;
  }
  SpawnFood();
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
}

void SnakeGame::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  Layout();
}

void SnakeGame::Update(double dt_ms, const InputState& input) {
  pixel_effects::UpdateParticles(particles_, kMaxParticles, dt_ms);
  pixel_effects::UpdateBursts(bursts_, kMaxBursts, dt_ms);
  if (game_over_) {
    if (input.Pressed(kActionPrimary)) Reset();
    return;
  }
  if (paused_) return;

  if (input.Pressed(kActionLeft) && dir_x_ == 0) {
    next_dir_x_ = -1; next_dir_y_ = 0;
  } else if (input.Pressed(kActionRight) && dir_x_ == 0) {
    next_dir_x_ = 1; next_dir_y_ = 0;
  } else if (input.Pressed(kActionUp) && dir_y_ == 0) {
    next_dir_x_ = 0; next_dir_y_ = -1;
  } else if (input.Pressed(kActionDown) && dir_y_ == 0) {
    next_dir_x_ = 0; next_dir_y_ = 1;
  }

  accumulator_ms_ += dt_ms;
  const double max_acc = step_ms_ * 3.0;
  if (accumulator_ms_ > max_acc) accumulator_ms_ = max_acc;
  while (accumulator_ms_ >= step_ms_ && !game_over_) {
    Step();
    accumulator_ms_ -= step_ms_;
  }
}

void SnakeGame::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  DrawBackground(renderer);
  DrawPlayfield(renderer);
  DrawHud(renderer);
  pixel_effects::DrawParticles(renderer, particles_, kMaxParticles,
                               arcade::DefaultPalette().pieces, 7);
  pixel_effects::DrawBursts(renderer, bursts_, kMaxBursts,
                            arcade::DefaultPalette().pieces, 7,
                            static_cast<float>(cell_ * 2), 3.0f);
  if (paused_ || game_over_) DrawOverlay(renderer);
}

void SnakeGame::SetPaused(bool paused) {
  if (game_over_ || paused_ == paused) return;
  paused_ = paused;
  QueueSound(kSfxPause);
}

void SnakeGame::ToggleSoundMute() { sound_muted_ = !sound_muted_; }

void SnakeGame::CycleDifficulty() {
  speed_ = arcade::CycleInt(speed_, 1, 9);
  step_ms_ = 230.0 - speed_ * 22.0;
  QueueSound(kSfxLevel);
}

uint32_t SnakeGame::ConsumeSoundEvents() {
  return arcade::ConsumeSoundEvents(&sound_events_);
}

void SnakeGame::Layout() {
  const arcade::GridSideLayout layout = arcade::ComputeGridSideLayout(
      width_, height_, kCols, kRows, 5, true);
  cell_ = layout.cell;
  play_x_ = layout.play_x; play_y_ = layout.play_y;
  play_w_ = layout.play_w; play_h_ = layout.play_h;
  border_ = layout.border;
  hud_x_ = layout.hud_x; hud_y_ = layout.hud_y;
  hud_w_ = layout.hud_w; hud_h_ = layout.hud_h;
}

void SnakeGame::SpawnFood() {
  for (int tries = 0; tries < 600; ++tries) {
    int x = static_cast<int>(arcade::NextRandom(&rng_) % kCols);
    int y = static_cast<int>(arcade::NextRandom(&rng_) % kRows);
    if (!Occupied(x, y)) {
      food_.x = x;
      food_.y = y;
      return;
    }
  }
  food_.x = 0;
  food_.y = 0;
}

void SnakeGame::Step() {
  dir_x_ = next_dir_x_;
  dir_y_ = next_dir_y_;
  Cell head = {snake_[0].x + dir_x_, snake_[0].y + dir_y_};
  if (head.x < 0 || head.x >= kCols || head.y < 0 || head.y >= kRows ||
      Occupied(head.x, head.y)) {
    game_over_ = true;
    QueueSound(kSfxLose);
    // A wall hit uses the same pixel explosion as the other Snake impacts;
    // the expanding square burst is reserved for games that need it.
    pixel_effects::SpawnParticles(
        particles_, kMaxParticles,
        play_x_ + snake_[0].x * cell_ + cell_ / 2,
        play_y_ + snake_[0].y * cell_ + cell_ / 2, 6, 18, &rng_);
    return;
  }
  bool ate = head.x == food_.x && head.y == food_.y;
  int new_length = length_ + (ate && length_ < kMaxSegments ? 1 : 0);
  for (int i = new_length - 1; i > 0; --i) snake_[i] = snake_[i - 1];
  snake_[0] = head;
  length_ = new_length;
  if (ate) {
    score_ += 10 * speed_;
    QueueSound(kSfxBonus);
    pixel_effects::SpawnParticles(particles_, kMaxParticles,
                                  play_x_ + food_.x * cell_ + cell_ / 2,
                                  play_y_ + food_.y * cell_ + cell_ / 2, 4, 14,
                                  &rng_);
    SpawnFood();
  } else {
    QueueSound(kSfxBrick);
  }
}

void SnakeGame::QueueSound(ArcadeSfx sfx) {
  arcade::QueueSoundEvent(&sound_events_, sfx);
}

void SnakeGame::DrawBackground(Gles2Renderer* renderer) {
  renderer->BeginFrame(arcade::DefaultPalette().screen_bg);
  arcade::DrawBackgroundPattern(
      renderer,
      GlesRect{0, 0, static_cast<float>(width_), static_cast<float>(height_)},
      score_ / 50 + speed_ * 3 + length_, arcade::MaxInt(54, width_ / 16));
}

void SnakeGame::DrawPlayfield(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  const GlesRect frame = {
      static_cast<float>(play_x_ - border_),
      static_cast<float>(play_y_ - border_),
      static_cast<float>(play_w_ + border_ * 2),
      static_cast<float>(play_h_ + border_ * 2)};
  arcade::DrawPanel(renderer, frame, p.playfield_bg, p.board_border,
                    static_cast<float>(border_));
  renderer->SetBlendMode(kGlesBlendAlpha);
  GlesColor grid = p.grid; grid.a = 0.45f;
  for (int x = 1; x < kCols; ++x)
    renderer->FillRect(GlesRect{static_cast<float>(play_x_ + x * cell_),
                                static_cast<float>(play_y_), 1.0f,
                                static_cast<float>(play_h_)}, grid);
  for (int y = 1; y < kRows; ++y)
    renderer->FillRect(GlesRect{static_cast<float>(play_x_),
                                static_cast<float>(play_y_ + y * cell_),
                                static_cast<float>(play_w_), 1.0f}, grid);
  renderer->SetBlendMode(kGlesBlendOpaque);

  const AsciiSpritePalette food = MakeAsciiSpritePalette(
      arcade::Darken(p.pieces[6]), arcade::Darken(p.pieces[6]), p.pieces[6],
      p.block_highlight, p.block_highlight, p.pieces[6], p.pieces[6]);
  DrawAsciiSprite(renderer, neon_serpent_assets::kBall,
      GlesRect{static_cast<float>(play_x_ + food_.x * cell_ + 3),
               static_cast<float>(play_y_ + food_.y * cell_ + 3),
               static_cast<float>(cell_ - 6), static_cast<float>(cell_ - 6)}, food);
  const float segment_inset = 0.5f;
  for (int i = length_ - 1; i >= 0; --i) {
    const bool is_head = i == 0;
    // Every segment is a shaded bubble; only the head changes to orange.
    GlesColor color = is_head ? p.pieces[2] : p.pieces[4];
    const GlesColor light = arcade::Lighten(color);
    const AsciiSpritePalette ball = MakeAsciiSpritePalette(
        arcade::Darken(color), arcade::Darken(color), color, light, p.block_highlight, color,
        color);
    DrawAsciiSprite(renderer, neon_serpent_assets::kBall,
        GlesRect{static_cast<float>(play_x_ + snake_[i].x * cell_) +
                     segment_inset,
                 static_cast<float>(play_y_ + snake_[i].y * cell_) +
                     segment_inset,
                 static_cast<float>(cell_) - segment_inset * 2.0f,
                 static_cast<float>(cell_) - segment_inset * 2.0f},
        ball);
  }
  arcade::DrawBorder(renderer, frame, p.board_border,
                     static_cast<float>(border_));
}

void SnakeGame::DrawHud(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::HudLayout layout = arcade::ComputeHudLayout(
      GlesRect{static_cast<float>(hud_x_), static_cast<float>(hud_y_),
               static_cast<float>(hud_w_), static_cast<float>(hud_h_)}, 5);
  arcade::DrawHudPanel(renderer, layout, style);
  arcade::HudFlow flow = arcade::BeginHudFlow(layout);
  arcade::DrawHudNumber(renderer, &flow, layout, "SCORE", score_, 5,
                        p.score, style);
  arcade::DrawHudNumber(renderer, &flow, layout, "SPEED", speed_, 1,
                        p.speed, style);
  arcade::DrawHudTextValue(renderer, &flow, layout, "SOUND",
                           sound_muted_ ? "OFF" : "ON",
                           sound_muted_ ? p.game_over_border : p.score, style);
}

void SnakeGame::DrawOverlay(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawCenteredOverlay(
      renderer, GlesRect{static_cast<float>(play_x_), static_cast<float>(play_y_),
                         static_cast<float>(play_w_), static_cast<float>(play_h_)},
      game_over_ ? "GAME OVER" : "PAUSED",
      game_over_ ? p.game_over_border : p.pause_border,
      arcade::MaxInt(3, play_w_ / 180));
}

bool SnakeGame::Occupied(int x, int y) const {
  for (int i = 0; i < length_; ++i)
    if (snake_[i].x == x && snake_[i].y == y) return true;
  return false;
}
