#include "brickbound_game.h"
#include "brickbound_assets.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/collision.h"
#include "engine/src/color_util.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"

#include <math.h>
#include <string.h>

namespace {

float BallRadius(int play_width) {
  return static_cast<float>(arcade::MaxInt(14, play_width / 55));
}

enum BonusType {
  kBonusExpand = 0,
  kBonusMulti,
  kBonusSlow,
  kBonusLife,
  kBonusLaser,
  kBonusCount
};

struct BrickGridLayout {
  float x;
  float y;
  float brick_w;
  float brick_h;
  float gap;
};

BrickGridLayout MakeBrickGridLayout(int play_x, int play_y, int play_w,
                                    int play_h, int border, int cols) {
  const float gap = static_cast<float>(arcade::MaxInt(4, play_w / 230));
  const float inset = static_cast<float>(border * 2);
  const float usable_w = static_cast<float>(play_w) - inset * 2.0f;
  return BrickGridLayout{
      static_cast<float>(play_x) + inset,
      static_cast<float>(play_y + border * 6),
      (usable_w - gap * static_cast<float>(cols - 1)) /
          static_cast<float>(cols),
      static_cast<float>(arcade::MaxInt(22, play_h / 24)), gap};
}

}  // namespace

ArkanoidGlesGame::ArkanoidGlesGame()
    : width_(0),
      height_(0),
      play_x_(0),
      play_y_(0),
      play_w_(0),
      play_h_(0),
      hud_x_(0),
      hud_y_(0),
      hud_w_(0),
      hud_h_(0),
      border_(0),
      paddle_x_(0.0f),
      paddle_y_(0.0f),
      paddle_w_(0.0f),
      paddle_h_(0.0f),
      score_(0),
      lives_(3),
      level_(1),
      bricks_left_(0),
      paused_(false),
      game_over_(false),
      sound_muted_(false),
      laser_charges_(0),
      expand_ms_(0.0f),
      slow_ms_(0.0f),
      sound_events_(0),
      rng_(0xace051u) {
  memset(balls_, 0, sizeof(balls_));
  memset(bricks_, 0, sizeof(bricks_));
  memset(bonuses_, 0, sizeof(bonuses_));
  memset(lasers_, 0, sizeof(lasers_));
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
}

void ArkanoidGlesGame::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  Layout();
  if (bricks_left_ == 0) Reset();
}

void ArkanoidGlesGame::Layout() {
  int margin = arcade::ScreenMargin(width_, height_);
  int gap = arcade::SideHudGap(width_);
  hud_w_ = arcade::SideHudWidth(width_);
  border_ = arcade::PanelBorderSize(hud_w_);
  play_h_ = height_ - margin * 2;
  int avail_w = width_ - margin * 2 - gap - hud_w_;
  play_w_ = arcade::MinInt(avail_w, play_h_ * 15 / 16);
  int total_w = play_w_ + gap + hud_w_;
  play_x_ = (width_ - total_w) / 2;
  play_y_ = (height_ - play_h_) / 2;
  hud_x_ = play_x_ + play_w_ + gap;
  hud_y_ = play_y_;
  hud_h_ = play_h_;
  paddle_w_ = static_cast<float>(arcade::MaxInt(36, play_w_ / 5));
  paddle_h_ = static_cast<float>(arcade::MaxInt(10, play_h_ / 34));
  paddle_y_ = static_cast<float>(play_y_ + play_h_ - border_ * 2 - paddle_h_);
  if (paddle_x_ <= 0.0f) {
    paddle_x_ = play_x_ + (play_w_ - paddle_w_) * 0.5f;
  }
}

void ArkanoidGlesGame::Reset() {
  score_ = 0;
  lives_ = 3;
  level_ = 1;
  paused_ = false;
  game_over_ = false;
  laser_charges_ = 0;
  expand_ms_ = 0.0f;
  slow_ms_ = 0.0f;
  GenerateLevel();
  ServeBall();
}

void ArkanoidGlesGame::GenerateLevel() {
  bricks_left_ = 0;
  rng_ = 0x9e3779b9u ^ static_cast<uint32_t>(level_ * 7919);
  bool filled[kRows][kCols];
  memset(filled, 0, sizeof(filled));
  const int center = kCols / 2;
  const int half_cols = center + 1;
  const int style = level_ % 6;
  const bool has_tunnels = (level_ % 3) != 1;

  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < half_cols; ++x) {
      bool on = true;
      if (style == 0) {
        int d = arcade::AbsInt(x - center);
        on = y < kRows - d / 2;
      } else if (style == 1) {
        on = ((x + y + level_) % 4) != 0 || y < 2;
      } else if (style == 2) {
        on = (x + y >= 3) && (x * 2 + y < kCols + kRows - 2);
      } else if (style == 3) {
        on = y == 0 || y == kRows - 1 || x == 0 || x == center ||
             ((x + y) % 3 != 1);
      } else if (style == 4) {
        int wave = (y * 2 + level_) % half_cols;
        on = x <= wave || y < 2 || y > kRows - 3;
      } else {
        on = (x % 2 == 0) || (y % 3 != 1) || y < 2;
      }

      filled[y][x] = on;
      filled[y][kCols - 1 - x] = on;
    }
  }

  if (has_tunnels) {
    int row = 2 + (level_ * 2) % (kRows - 3);
    for (int x = 1; x < kCols - 1; ++x) filled[row][x] = false;

    int vertical_offset = 1 + (level_ % 2);
    int left_tunnel = center - vertical_offset;
    int right_tunnel = center + vertical_offset;
    for (int y = 1; y < kRows - 1; ++y) {
      if ((y + level_) % 4 == 0) continue;
      filled[y][left_tunnel] = false;
      filled[y][right_tunnel] = false;
    }
    filled[row][center] = false;
  } else {
    int notch_row = 1 + (level_ % (kRows - 2));
    filled[notch_row][center] = false;
  }

  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      bool hole = !filled[y][x];
      bricks_[y][x].hp = hole ? 0 : 1;
      bricks_[y][x].color = (arcade::AbsInt(x - center) + y + level_) % 7;
      bricks_[y][x].bonus = (!hole && (arcade::NextRandom(&rng_) % 100) < 18)
                                 ? static_cast<int>(arcade::NextRandom(&rng_) % kBonusCount)
                                 : -1;
      if (bricks_[y][x].hp > 0) ++bricks_left_;
    }
  }

  if (bricks_left_ < 24) {
    for (int y = 0; y < kRows; ++y) {
      int x = center - (y % 3);
      int mirror = kCols - 1 - x;
      if (bricks_[y][x].hp == 0) {
        bricks_[y][x].hp = 1;
        bricks_[y][x].color = (y + level_) % 7;
        bricks_[y][x].bonus = -1;
        ++bricks_left_;
      }
      if (mirror != x && bricks_[y][mirror].hp == 0) {
        bricks_[y][mirror].hp = 1;
        bricks_[y][mirror].color = (y + level_) % 7;
        bricks_[y][mirror].bonus = -1;
        ++bricks_left_;
      }
    }
  }
}

void ArkanoidGlesGame::Update(double dt_ms, const InputState& input) {
  if (input.Pressed(kActionPrimary)) {
    if (game_over_) Reset();
    else LaunchBalls();
  }
  if (input.Pressed(kActionPause) || input.Pressed(kActionBack)) {
    TogglePauseFromSystem();
  }
  if (paused_ && input.Pressed(kActionDown)) {
    ToggleSoundMute();
  }
  if (input.Pressed(kActionUp) || input.Pressed(kActionSecondary)) FireLaser();
  if (paused_ || game_over_) {
    UpdateEffects(dt_ms);
    return;
  }

  float dt = static_cast<float>(dt_ms / 1000.0);
  float speed = 620.0f + level_ * 24.0f;
  if (input.Down(kActionLeft)) paddle_x_ -= speed * dt;
  if (input.Down(kActionRight)) paddle_x_ += speed * dt;
  paddle_x_ = arcade::ClampFloat(paddle_x_, play_x_ + border_,
                         play_x_ + play_w_ - border_ - paddle_w_);
  if (expand_ms_ > 0.0f) {
    expand_ms_ -= static_cast<float>(dt_ms);
    if (expand_ms_ <= 0.0f) paddle_w_ = static_cast<float>(arcade::MaxInt(100, play_w_ / 5));
  }
  if (slow_ms_ > 0.0f) slow_ms_ -= static_cast<float>(dt_ms);

  for (int i = 0; i < kMaxBalls; ++i) {
    if (balls_[i].active) MoveBall(&balls_[i], dt);
  }
  UpdateBonuses(dt);
  UpdateLasers(dt);
  int active = 0;
  for (int i = 0; i < kMaxBalls; ++i)
    if (balls_[i].active) ++active;
  if (active == 0) LoseLife();
  UpdateEffects(dt_ms);
}

void ArkanoidGlesGame::ServeBall() {
  memset(balls_, 0, sizeof(balls_));
  memset(bonuses_, 0, sizeof(bonuses_));
  memset(lasers_, 0, sizeof(lasers_));
  balls_[0].active = true;
  balls_[0].stuck = true;
  balls_[0].x = paddle_x_ + paddle_w_ * 0.5f;
  balls_[0].y = paddle_y_ - 16.0f;
}

void ArkanoidGlesGame::LaunchBalls() {
  for (int i = 0; i < kMaxBalls; ++i) {
    if (balls_[i].active && balls_[i].stuck) {
      balls_[i].stuck = false;
      balls_[i].vx = ((i + level_) & 1 ? 260.0f : -260.0f);
      balls_[i].vy = -470.0f - level_ * 18.0f;
    }
  }
}

void ArkanoidGlesGame::MoveBall(Ball* b, double dt) {
  float radius = BallRadius(play_w_);
  if (b->stuck) {
    b->x = paddle_x_ + paddle_w_ * 0.5f;
    b->y = paddle_y_ - radius - 2.0f;
    return;
  }
  float slow = slow_ms_ > 0.0f ? 0.72f : 1.0f;
  const float travel = arcade::MaxFloat(fabsf(b->vx), fabsf(b->vy)) *
                       static_cast<float>(dt) * slow;
  const float max_step = arcade::MaxFloat(4.0f, radius * 0.5f);
  const int steps = arcade::MaxInt(1, static_cast<int>(ceilf(travel / max_step)));
  const float step_dt = static_cast<float>(dt) / static_cast<float>(steps);
  for (int i = 0; i < steps && b->active && !b->stuck; ++i) {
    MoveBallStep(b, step_dt, slow);
  }
}

void ArkanoidGlesGame::MoveBallStep(Ball* b, float dt, float slow) {
  const float radius = BallRadius(play_w_);
  b->x += b->vx * dt * slow;
  b->y += b->vy * dt * slow;
  if (b->x - radius <= play_x_ + border_) {
    b->x = play_x_ + border_ + radius;
    b->vx = fabsf(b->vx);
  }
  if (b->x + radius >= play_x_ + play_w_ - border_) {
    b->x = play_x_ + play_w_ - border_ - radius;
    b->vx = -fabsf(b->vx);
  }
  if (b->y - radius <= play_y_ + border_) {
    b->y = play_y_ + border_ + radius;
    b->vy = fabsf(b->vy);
  }
  if (b->y + radius >= paddle_y_ && b->y - radius <= paddle_y_ + paddle_h_ &&
      b->x >= paddle_x_ && b->x <= paddle_x_ + paddle_w_ && b->vy > 0.0f) {
    float hit = (b->x - (paddle_x_ + paddle_w_ * 0.5f)) / (paddle_w_ * 0.5f);
    b->vx = hit * (420.0f + level_ * 18.0f);
    b->vy = -fabsf(500.0f + level_ * 20.0f);
    b->y = paddle_y_ - radius - 1.0f;
    SpawnParticles(b->x, b->y, 3, 8);
    QueueSound(kSfxPaddle);
  }

  const BrickGridLayout grid = MakeBrickGridLayout(
      play_x_, play_y_, play_w_, play_h_, border_, kCols);
  int hit_col = -1;
  int hit_row = -1;
  arcade::CircleRectHit best_hit = {};
  float best_penetration = -1.0f;
  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      if (bricks_[row][col].hp <= 0) continue;
      const arcade::CollisionRect rect = {
          grid.x + col * (grid.brick_w + grid.gap),
          grid.y + row * (grid.brick_h + grid.gap),
          grid.brick_w, grid.brick_h};
      arcade::CircleRectHit hit = {};
      if (!arcade::CircleIntersectsRect(b->x, b->y, radius, rect, &hit)) {
        continue;
      }
      const float approach = b->vx * hit.normal_x + b->vy * hit.normal_y;
      if (approach >= 0.0f || hit.penetration <= best_penetration) continue;
      hit_col = col;
      hit_row = row;
      best_hit = hit;
      best_penetration = hit.penetration;
    }
  }
  if (hit_col >= 0) {
    b->x += best_hit.normal_x * (best_hit.penetration + 0.01f);
    b->y += best_hit.normal_y * (best_hit.penetration + 0.01f);
    const float approach =
        b->vx * best_hit.normal_x + b->vy * best_hit.normal_y;
    b->vx -= 2.0f * approach * best_hit.normal_x;
    b->vy -= 2.0f * approach * best_hit.normal_y;
    HitBrick(hit_col, hit_row);
  }
  if (b->y - radius > play_y_ + play_h_) b->active = false;
}

void ArkanoidGlesGame::HitBrick(int col, int row) {
  Brick& brick = bricks_[row][col];
  if (brick.hp <= 0) return;
  --brick.hp;
  score_ += 7 + level_;
  const BrickGridLayout grid = MakeBrickGridLayout(
      play_x_, play_y_, play_w_, play_h_, border_, kCols);
  float x = grid.x + col * (grid.brick_w + grid.gap) + grid.brick_w * 0.5f;
  float y = grid.y + row * (grid.brick_h + grid.gap) + grid.brick_h * 0.5f;
  SpawnParticles(x, y, brick.color, brick.hp <= 0 ? 18 : 8);
  SpawnBurst(x, y, brick.color);
  QueueSound(kSfxBrick);
  if (brick.hp <= 0) {
    --bricks_left_;
    if (brick.bonus >= 0) SpawnBonus(x, y, brick.bonus);
    if (bricks_left_ <= 0) NextLevel();
  }
}

void ArkanoidGlesGame::FireLaser() {
  if (laser_charges_ <= 0) return;
  for (int i = 0; i < kMaxLasers; ++i) {
    if (lasers_[i].active) continue;
    lasers_[i].active = true;
    lasers_[i].x = paddle_x_ + paddle_w_ * 0.5f;
    lasers_[i].y = paddle_y_;
    --laser_charges_;
    SpawnParticles(lasers_[i].x, lasers_[i].y, 0, 10);
    QueueSound(kSfxLaser);
    return;
  }
}

void ArkanoidGlesGame::SpawnBonus(float x, float y, int type) {
  for (int i = 0; i < kMaxBonuses; ++i) {
    if (bonuses_[i].active) continue;
    bonuses_[i].active = true;
    bonuses_[i].x = x;
    bonuses_[i].y = y;
    bonuses_[i].vy = 135.0f + level_ * 6.0f;
    bonuses_[i].type = type;
    return;
  }
}

void ArkanoidGlesGame::ApplyBonus(int type) {
  if (type == kBonusExpand) {
    paddle_w_ = arcade::MinFloat(play_w_ * 0.34f, paddle_w_ * 1.45f);
    expand_ms_ = 12000.0f;
  } else if (type == kBonusMulti) {
    for (int i = 0; i < kMaxBalls; ++i) {
      if (!balls_[i].active || balls_[i].stuck) continue;
      for (int n = 0; n < kMaxBalls; ++n) {
        if (balls_[n].active) continue;
        balls_[n] = balls_[i];
        balls_[n].vx = -balls_[i].vx;
        balls_[n].vy = -fabsf(balls_[i].vy);
        break;
      }
      break;
    }
  } else if (type == kBonusSlow) {
    slow_ms_ = 9000.0f;
  } else if (type == kBonusLife) {
    lives_ = arcade::MinInt(9, lives_ + 1);
  } else if (type == kBonusLaser) {
    laser_charges_ = arcade::MinInt(9, laser_charges_ + 3);
  }
}

void ArkanoidGlesGame::UpdateBonuses(double dt) {
  float size = static_cast<float>(arcade::MaxInt(24, play_w_ / 34));
  for (int i = 0; i < kMaxBonuses; ++i) {
    Bonus& b = bonuses_[i];
    if (!b.active) continue;
    b.y += b.vy * static_cast<float>(dt);
    if (b.y + size >= paddle_y_ && b.y <= paddle_y_ + paddle_h_ &&
        b.x + size >= paddle_x_ && b.x <= paddle_x_ + paddle_w_) {
      ApplyBonus(b.type);
      SpawnBurst(b.x + size * 0.5f, b.y + size * 0.5f, b.type % 7);
      QueueSound(b.type == kBonusLife ? kSfxLife : kSfxBonus);
      b.active = false;
    }
    if (b.y > play_y_ + play_h_) b.active = false;
  }
}

void ArkanoidGlesGame::UpdateLasers(double dt) {
  const BrickGridLayout grid = MakeBrickGridLayout(
      play_x_, play_y_, play_w_, play_h_, border_, kCols);
  for (int i = 0; i < kMaxLasers; ++i) {
    Laser& l = lasers_[i];
    if (!l.active) continue;
    l.y -= static_cast<float>(820.0 * dt);
    int col = static_cast<int>((l.x - grid.x) / (grid.brick_w + grid.gap));
    int row = static_cast<int>((l.y - grid.y) / (grid.brick_h + grid.gap));
    if (col >= 0 && col < kCols && row >= 0 && row < kRows &&
        bricks_[row][col].hp > 0) {
      HitBrick(col, row);
      l.active = false;
    }
    if (l.y < play_y_) l.active = false;
  }
}

void ArkanoidGlesGame::LoseLife() {
  --lives_;
  SpawnBurst(paddle_x_ + paddle_w_ * 0.5f, paddle_y_, 6);
  QueueSound(kSfxLose);
  if (lives_ <= 0) {
    game_over_ = true;
    return;
  }
  ServeBall();
}

void ArkanoidGlesGame::NextLevel() {
  ++level_;
  GenerateLevel();
  QueueSound(kSfxLevel);
  ServeBall();
}

void ArkanoidGlesGame::SpawnParticles(float x, float y, int color, int count) {
  pixel_effects::SpawnParticles(particles_, kMaxParticles, x, y, color, count,
                                &rng_);
}

void ArkanoidGlesGame::SpawnBurst(float x, float y, int color) {
  pixel_effects::SpawnBurst(bursts_, kMaxBursts, x, y, color);
}

void ArkanoidGlesGame::UpdateEffects(double dt_ms) {
  pixel_effects::UpdateParticles(particles_, kMaxParticles, dt_ms);
  pixel_effects::UpdateBursts(bursts_, kMaxBursts, dt_ms);
}

void ArkanoidGlesGame::QueueSound(ArcadeSfx sfx) {
  if (sound_muted_) return;
  arcade::QueueSoundEvent(&sound_events_, sfx);
}

void ArkanoidGlesGame::TogglePauseFromSystem() {
  if (game_over_) return;
  SetPaused(!paused_);
}

void ArkanoidGlesGame::SetPaused(bool paused) {
  if (game_over_ || paused_ == paused) return;
  paused_ = paused;
  QueueSound(kSfxPause);
}

void ArkanoidGlesGame::ToggleSoundMute() {
  if (!paused_ || game_over_) return;
  sound_muted_ = !sound_muted_;
  QueueSound(kSfxPause);
}

void ArkanoidGlesGame::CycleDifficulty() {
  if (!paused_ || game_over_) return;
  level_ = arcade::CycleInt(level_, 1, 9);
  GenerateLevel();
  ServeBall();
  QueueSound(kSfxLevel);
}

uint32_t ArkanoidGlesGame::ConsumeSoundEvents() {
  return arcade::ConsumeSoundEvents(&sound_events_);
}

void ArkanoidGlesGame::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  DrawBackground(renderer);
  DrawPlayfield(renderer);
  DrawHud(renderer);
  DrawOverlay(renderer);
}

void ArkanoidGlesGame::DrawBackground(Gles2Renderer* r) {
  const arcade::Palette& p = arcade::DefaultPalette();
  r->BeginFrame(p.screen_bg);
  int step = arcade::MaxInt(70, height_ / 10);
  for (int y = -step; y < height_ + step; y += step) {
    for (int x = -step; x < width_ + step; x += step) {
      int variant = ((x / step) + (y / step) + level_) & 3;
      GlesColor c = variant & 1 ? arcade::Rgb(78, 105, 121)
                                : arcade::Rgb(95, 139, 123);
      if (variant == 0) {
        r->FillRect(GlesRect{static_cast<float>(x + step / 5),
                             static_cast<float>(y + step / 5),
                             static_cast<float>(step / 2),
                             static_cast<float>(step / 2)}, c);
      } else if (variant == 1) {
        arcade::DrawPanel(r, GlesRect{static_cast<float>(x + step / 8),
                                      static_cast<float>(y + step / 8),
                                      static_cast<float>(step * 2 / 3),
                                      static_cast<float>(step * 2 / 3)},
                          p.screen_bg, c, arcade::MaxInt(5, step / 12));
      } else {
        r->FillRect(GlesRect{static_cast<float>(x + step / 2),
                             static_cast<float>(y + step / 6),
                             static_cast<float>(arcade::MaxInt(7, step / 10)),
                             static_cast<float>(step * 2 / 3)}, c);
        r->FillRect(GlesRect{static_cast<float>(x + step / 4),
                             static_cast<float>(y + step / 2),
                             static_cast<float>(step * 2 / 3),
                             static_cast<float>(arcade::MaxInt(7, step / 10))}, c);
      }
    }
  }
}

void ArkanoidGlesGame::DrawPlayfield(Gles2Renderer* r) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawPanel(r, GlesRect{static_cast<float>(play_x_),
                                static_cast<float>(play_y_),
                                static_cast<float>(play_w_),
                                static_cast<float>(play_h_)},
                    p.playfield_bg, p.board_border, border_);
  const BrickGridLayout grid = MakeBrickGridLayout(
      play_x_, play_y_, play_w_, play_h_, border_, kCols);
  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      const Brick& b = bricks_[row][col];
      if (b.hp <= 0) continue;
      GlesRect rect{grid.x + col * (grid.brick_w + grid.gap),
                    grid.y + row * (grid.brick_h + grid.gap), grid.brick_w,
                    grid.brick_h};
      const GlesColor fill = p.pieces[b.color];
      const AsciiSpritePalette brick = MakeAsciiSpritePalette(
          arcade::Darken(fill), arcade::Darken(fill), fill, arcade::Lighten(fill),
          p.block_highlight, fill, fill);
      DrawAsciiSprite(r, brickbound_assets::kBrick, rect, brick);
      if (b.hp > 1) {
        r->FillRect(GlesRect{rect.x + rect.w - 10.0f, rect.y + 5.0f, 5.0f,
                             rect.h - 10.0f},
                    p.overlay_text);
      }
    }
  }
  const AsciiSpritePalette paddle = MakeAsciiSpritePalette(
      arcade::Darken(p.score), arcade::Darken(p.score), p.score, p.block_highlight,
      p.block_highlight, p.score, p.score);
  DrawAsciiSprite(r, brickbound_assets::kPaddle,
                  GlesRect{paddle_x_, paddle_y_, paddle_w_, paddle_h_}, paddle);
  for (int i = 0; i < kMaxBonuses; ++i) {
    const Bonus& bonus = bonuses_[i];
    if (!bonus.active) continue;
    float size = static_cast<float>(arcade::MaxInt(30, play_w_ / 28));
    const GlesColor bonus_colors[kBonusCount] = {
        p.pieces[3], p.pieces[5], p.pieces[0], p.pieces[6], p.pieces[2]};
    GlesColor fill = bonus_colors[bonus.type];
    const AsciiSpritePalette sprite = MakeAsciiSpritePalette(
        arcade::Darken(fill), arcade::Darken(fill), fill, arcade::Lighten(fill),
        p.block_highlight, fill, fill);
    DrawAsciiSprite(r, brickbound_assets::kBall,
                    GlesRect{bonus.x, bonus.y, size, size}, sprite);
    const char* label = "E";
    if (bonus.type == kBonusMulti) label = "M";
    if (bonus.type == kBonusSlow) label = "S";
    if (bonus.type == kBonusLife) label = "L";
    if (bonus.type == kBonusLaser) label = "X";
    const int label_scale = 2;
    const int label_width = arcade::PixelTextWidth(label, label_scale);
    const GlesColor label_color = arcade::ColorLuma(fill) > 0.62f
        ? p.overlay_panel : p.overlay_text;
    arcade::DrawPixelText(r, label,
                          static_cast<int>(bonus.x + (size - label_width) * 0.5f),
                          static_cast<int>(bonus.y + (size - 7 * label_scale) * 0.5f),
                          label_scale, label_color);
  }
  for (int i = 0; i < kMaxLasers; ++i) {
    const Laser& laser = lasers_[i];
    if (!laser.active) continue;
    r->FillRect(GlesRect{laser.x - 3.0f, laser.y - 34.0f, 6.0f, 34.0f},
                p.pieces[0]);
    r->FillRect(GlesRect{laser.x - 1.0f, laser.y - 34.0f, 2.0f, 34.0f},
                p.block_highlight);
  }
  r->SetBlendMode(kGlesBlendAlpha);
  pixel_effects::DrawBursts(r, bursts_, kMaxBursts, p.pieces, 7, 70.0f,
                            static_cast<float>(arcade::MaxInt(3, play_w_ / 180)));
  pixel_effects::DrawParticles(r, particles_, kMaxParticles, p.pieces, 7);
  r->SetBlendMode(kGlesBlendOpaque);
  for (int i = 0; i < kMaxBalls; ++i) {
    const Ball& b = balls_[i];
    if (!b.active) continue;
    float radius = BallRadius(play_w_);
    const GlesColor ball_color = p.pieces[3];
    const AsciiSpritePalette ball = MakeAsciiSpritePalette(
        arcade::Darken(ball_color), arcade::Darken(ball_color), ball_color,
        arcade::Lighten(ball_color), p.block_highlight, ball_color, ball_color);
    DrawAsciiSprite(r, brickbound_assets::kBall,
                    GlesRect{b.x - radius, b.y - radius,
                             radius * 2.0f, radius * 2.0f}, ball);
  }
}

void ArkanoidGlesGame::DrawHud(Gles2Renderer* r) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::HudLayout layout = arcade::ComputeHudLayout(
      GlesRect{static_cast<float>(hud_x_), static_cast<float>(hud_y_),
               static_cast<float>(hud_w_), static_cast<float>(hud_h_)}, 4);
  arcade::DrawHudPanel(r, layout, style);
  arcade::HudFlow flow = arcade::BeginHudFlow(layout);
  arcade::DrawHudNumber(r, &flow, layout, "SCORE", score_, 4, p.score, style);
  arcade::DrawHudNumber(r, &flow, layout, "LEVEL", level_, 2, p.speed, style);
  arcade::DrawHudNumber(r, &flow, layout, "LIVES", lives_, 1, p.score, style);
  arcade::DrawHudNumber(r, &flow, layout, "LASER", laser_charges_, 1,
                        p.pieces[0], style);
  arcade::DrawHudTextValue(r, &flow, layout, "SOUND",
                           sound_muted_ ? "OFF" : "ON",
                           sound_muted_ ? p.game_over_border : p.score, style);
}

void ArkanoidGlesGame::DrawOverlay(Gles2Renderer* r) {
  if (!paused_ && !game_over_) return;
  const arcade::Palette& p = arcade::DefaultPalette();
  const char* text = game_over_ ? "GAME OVER" : "PAUSED";
  arcade::DrawCenteredOverlay(
      r, GlesRect{static_cast<float>(play_x_), static_cast<float>(play_y_),
                  static_cast<float>(play_w_), static_cast<float>(play_h_)},
      text,
      game_over_ ? p.game_over_border : p.pause_border,
      arcade::MaxInt(3, play_w_ / 180));
}
