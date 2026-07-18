#include "game_2048.h"
#include "game_2048_assets.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/color_util.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"

#include <stdio.h>
#include <string.h>

Game2048::Game2048()
    : width_(1280),
      height_(720),
      board_x_(0),
      board_y_(0),
      board_size_(0),
      cell_(0),
      gap_(0),
      hud_x_(0),
      hud_y_(0),
      hud_w_(0),
      hud_h_(0),
      score_(0),
      moves_(0),
      difficulty_(1),
      paused_(false),
      game_over_(false),
      won_(false),
      win_notice_(false),
      sound_muted_(false),
      pattern_stage_(0),
      sound_events_(0),
      rng_(0x2048cafeu) {
  memset(tiles_, 0, sizeof(tiles_));
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  Reset();
}

void Game2048::Reset() {
  score_ = 0;
  moves_ = 0;
  paused_ = false;
  game_over_ = false;
  won_ = false;
  win_notice_ = false;
  pattern_stage_ = 0;
  sound_events_ = 0;
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  ClearBoard();
  AddRandomTile();
  AddRandomTile();
}

void Game2048::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  Layout();
}

void Game2048::Update(double dt_ms, const InputState& input) {
  UpdateEffects(dt_ms);
  if (game_over_) {
    if (input.Pressed(kActionPrimary)) Reset();
    return;
  }
  if (win_notice_) {
    if (input.Pressed(kActionPrimary) || input.Pressed(kActionLeft) ||
        input.Pressed(kActionRight) || input.Pressed(kActionUp) ||
        input.Pressed(kActionDown)) {
      win_notice_ = false;
    } else {
      return;
    }
  }
  if (paused_) return;

  Direction direction = kDirLeft;
  bool pressed = false;
  if (input.Pressed(kActionLeft)) {
    direction = kDirLeft;
    pressed = true;
  } else if (input.Pressed(kActionRight)) {
    direction = kDirRight;
    pressed = true;
  } else if (input.Pressed(kActionUp)) {
    direction = kDirUp;
    pressed = true;
  } else if (input.Pressed(kActionDown)) {
    direction = kDirDown;
    pressed = true;
  }

  if (!pressed) return;
  MoveResult result = Move(direction);
  if (!result.moved) {
    QueueSound(kSfxPaddle);
    return;
  }

  score_ += result.gained;
  ++moves_;
  QueueSound(result.merged ? kSfxBonus : kSfxBrick);
  AddRandomTile();
  int next_stage = PatternStageForTile(BestTile());
  if (next_stage > pattern_stage_) pattern_stage_ = next_stage;
  if (BestTile() >= 2048 && !won_) {
    won_ = true;
    win_notice_ = true;
    QueueSound(kSfxLevel);
  }
  if (!HasMoves()) {
    game_over_ = true;
    QueueSound(kSfxLose);
  }
}

void Game2048::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  DrawBackground(renderer);
  DrawBoard(renderer);
  DrawHud(renderer);
  pixel_effects::DrawParticles(renderer, particles_, kMaxParticles,
                               arcade::DefaultPalette().pieces, 7);
  pixel_effects::DrawBursts(renderer, bursts_, kMaxBursts,
                            arcade::DefaultPalette().pieces, 7,
                            static_cast<float>(cell_ / 2), 4.0f);
  if (paused_ || game_over_ || win_notice_) DrawOverlay(renderer);
}

void Game2048::SetPaused(bool paused) {
  if (game_over_) return;
  if (paused_ == paused) return;
  paused_ = paused;
  QueueSound(kSfxPause);
}

void Game2048::ToggleSoundMute() { sound_muted_ = !sound_muted_; }

void Game2048::CycleDifficulty() {
  difficulty_ = arcade::CycleInt(difficulty_, 1, 9);
  QueueSound(kSfxLevel);
}

uint32_t Game2048::ConsumeSoundEvents() {
  return arcade::ConsumeSoundEvents(&sound_events_);
}

void Game2048::Layout() {
  const int margin = arcade::ScreenMargin(width_, height_);
  const int hud_gap = arcade::SideHudGap(width_);
  const int hud_width = arcade::SideHudWidth(width_);
  const int max_board_h = height_ - margin * 2;
  const int max_board_w = width_ - margin * 2 - hud_gap - hud_width;
  board_size_ = arcade::MinInt(max_board_h, max_board_w);
  board_size_ = arcade::MaxInt(96, board_size_);
  gap_ = arcade::MaxInt(8, board_size_ / 42);
  cell_ = (board_size_ - gap_ * 5) / 4;
  board_size_ = cell_ * 4 + gap_ * 5;
  board_x_ = (width_ - board_size_ - hud_gap - hud_width) / 2;
  if (board_x_ < margin) board_x_ = margin;
  board_y_ = (height_ - board_size_) / 2;
  hud_x_ = board_x_ + board_size_ + hud_gap;
  hud_y_ = board_y_;
  hud_w_ = hud_width;
  hud_h_ = board_size_;
}

void Game2048::ClearBoard() { memset(tiles_, 0, sizeof(tiles_)); }

void Game2048::AddRandomTile() {
  int empty = 0;
  for (int y = 0; y < kSize; ++y)
    for (int x = 0; x < kSize; ++x)
      if (tiles_[y][x] == 0) ++empty;
  if (!empty) return;
  int pick = static_cast<int>(arcade::NextRandom(&rng_) % empty);
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      if (tiles_[y][x] != 0) continue;
      if (pick-- == 0) {
        int value = (arcade::NextRandom(&rng_) % 10 == 0) ? 4 : 2;
        if (difficulty_ >= 7 && arcade::NextRandom(&rng_) % 7 == 0) value = 4;
        tiles_[y][x] = value;
        SpawnTileEffect(x, y, TileColorIndex(value));
        return;
      }
    }
  }
}

Game2048::MoveResult Game2048::Move(Direction direction) {
  MoveResult result = {false, false, 0};
  int next[kSize][kSize];
  memset(next, 0, sizeof(next));

  for (int line = 0; line < kSize; ++line) {
    int values[kSize] = {0, 0, 0, 0};
    int count = 0;
    for (int step = 0; step < kSize; ++step) {
      int x = 0;
      int y = 0;
      if (direction == kDirLeft || direction == kDirRight) {
        y = line;
        x = direction == kDirLeft ? step : kSize - 1 - step;
      } else {
        x = line;
        y = direction == kDirUp ? step : kSize - 1 - step;
      }
      if (tiles_[y][x] != 0) values[count++] = tiles_[y][x];
    }

    int merged[kSize] = {0, 0, 0, 0};
    int out[kSize] = {0, 0, 0, 0};
    int out_count = 0;
    for (int i = 0; i < count; ++i) {
      if (i + 1 < count && values[i] == values[i + 1]) {
        out[out_count] = values[i] * 2;
        merged[out_count] = 1;
        result.gained += out[out_count];
        result.merged = true;
        ++i;
      } else {
        out[out_count] = values[i];
      }
      ++out_count;
    }

    for (int step = 0; step < kSize; ++step) {
      int x = 0;
      int y = 0;
      if (direction == kDirLeft || direction == kDirRight) {
        y = line;
        x = direction == kDirLeft ? step : kSize - 1 - step;
      } else {
        x = line;
        y = direction == kDirUp ? step : kSize - 1 - step;
      }
      next[y][x] = out[step];
      if (merged[step]) SpawnMergeEffect(x, y, TileColorIndex(out[step]));
    }
  }

  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      if (tiles_[y][x] != next[y][x]) result.moved = true;
      tiles_[y][x] = next[y][x];
    }
  }
  return result;
}

bool Game2048::HasMoves() const {
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      if (tiles_[y][x] == 0) return true;
      if (x + 1 < kSize && tiles_[y][x] == tiles_[y][x + 1]) return true;
      if (y + 1 < kSize && tiles_[y][x] == tiles_[y + 1][x]) return true;
    }
  }
  return false;
}

int Game2048::BestTile() const {
  int best = 0;
  for (int y = 0; y < kSize; ++y)
    for (int x = 0; x < kSize; ++x)
      if (tiles_[y][x] > best) best = tiles_[y][x];
  return best;
}

int Game2048::PatternStageForTile(int value) const {
  if (value >= 2048) return 7;
  if (value >= 1024) return 6;
  if (value >= 512) return 5;
  if (value >= 256) return 4;
  if (value >= 128) return 3;
  if (value >= 64) return 2;
  if (value >= 32) return 1;
  return 0;
}

int Game2048::TileColorIndex(int value) const {
  int index = 0;
  while (value > 2) {
    value /= 2;
    ++index;
  }
  return index % 7;
}

void Game2048::SpawnTileEffect(int col, int row, int color) {
  const float x = static_cast<float>(board_x_ + gap_ + col * (cell_ + gap_) +
                                     cell_ / 2);
  const float y = static_cast<float>(board_y_ + gap_ + row * (cell_ + gap_) +
                                     cell_ / 2);
  (void)x;
  (void)y;
  (void)color;
}

void Game2048::SpawnMergeEffect(int col, int row, int color) {
  const float x = static_cast<float>(board_x_ + gap_ + col * (cell_ + gap_) +
                                     cell_ / 2);
  const float y = static_cast<float>(board_y_ + gap_ + row * (cell_ + gap_) +
                                     cell_ / 2);
  pixel_effects::SpawnParticles(particles_, kMaxParticles, x, y, color, 16,
                                &rng_);
  pixel_effects::SpawnBurst(bursts_, kMaxBursts, x, y, color);
}

void Game2048::UpdateEffects(double dt_ms) {
  pixel_effects::UpdateParticles(particles_, kMaxParticles, dt_ms);
  pixel_effects::UpdateBursts(bursts_, kMaxBursts, dt_ms);
}

void Game2048::QueueSound(ArcadeSfx sfx) {
  if (sfx < 0 || sfx >= kSfxCount) return;
  arcade::QueueSoundEvent(&sound_events_, sfx);
}

void Game2048::DrawBackground(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  renderer->BeginFrame(p.screen_bg);
  renderer->FillRect(GlesRect{0.0f, 0.0f, static_cast<float>(width_),
                              static_cast<float>(height_)},
                     p.screen_bg);
  const int step = arcade::MaxInt(48, width_ / (17 - arcade::MinInt(pattern_stage_, 5)));
  for (int y = -step; y < height_ + step; y += step) {
    for (int x = -step; x < width_ + step; x += step) {
      int gx = x / step;
      int gy = y / step;
      int phase = (gx * (pattern_stage_ + 1) + gy * 3 + pattern_stage_) & 3;
      GlesColor c = phase == 0 ? p.grid :
                    (phase == 1 ? p.hud_border :
                     (phase == 2 ? p.label : p.pieces[pattern_stage_ % 7]));
      c = arcade::MixColors(c, p.overlay_text, 0.16f);
      c.a = 0.34f;
      renderer->SetBlendMode(kGlesBlendAlpha);
      if ((pattern_stage_ & 1) && phase == 3) {
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 3),
                                    static_cast<float>(y + step / 5),
                                    static_cast<float>(step / 10),
                                    static_cast<float>(step / 2)},
                           c);
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 3),
                                    static_cast<float>(y + step / 5),
                                    static_cast<float>(step / 2),
                                    static_cast<float>(step / 10)},
                           c);
      } else if (phase == 2) {
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 3),
                                    static_cast<float>(y + step / 3),
                                    static_cast<float>(step / 3),
                                    static_cast<float>(step / 3)},
                           c);
      } else {
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 5),
                                    static_cast<float>(y + step / 2),
                                    static_cast<float>(step / 2),
                                    static_cast<float>(step / 8)},
                           c);
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 2),
                                    static_cast<float>(y + step / 5),
                                    static_cast<float>(step / 8),
                                    static_cast<float>(step / 2)},
                           c);
      }
      renderer->SetBlendMode(kGlesBlendOpaque);
    }
  }
}

void Game2048::DrawBoard(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  GlesColor board_glow = p.board_border;
  board_glow.a = 0.16f;
  renderer->SetBlendMode(kGlesBlendAlpha);
  renderer->FillRect(GlesRect{static_cast<float>(board_x_ - 8),
                              static_cast<float>(board_y_ - 8),
                              static_cast<float>(board_size_ + 16),
                              static_cast<float>(board_size_ + 16)},
                     board_glow);
  renderer->SetBlendMode(kGlesBlendOpaque);
  arcade::DrawPanel(renderer,
                    GlesRect{static_cast<float>(board_x_),
                             static_cast<float>(board_y_),
                             static_cast<float>(board_size_),
                             static_cast<float>(board_size_)},
                    p.playfield_bg, p.board_border,
                    static_cast<float>(arcade::PanelBorderSize(hud_w_)));
  for (int y = 0; y < kSize; ++y) {
    for (int x = 0; x < kSize; ++x) {
      float rx = static_cast<float>(board_x_ + gap_ + x * (cell_ + gap_));
      float ry = static_cast<float>(board_y_ + gap_ + y * (cell_ + gap_));
      renderer->FillRect(GlesRect{rx, ry, static_cast<float>(cell_),
                                  static_cast<float>(cell_)},
                         arcade::MixColors(p.hud_panel, p.playfield_bg, 0.28f));
      // A small inset frame gives empty cells the same arcade depth as the
      // active tiles without competing with their numbers.
      GlesColor slot = p.grid;
      slot.a = 0.42f;
      renderer->SetBlendMode(kGlesBlendAlpha);
      renderer->FillRect(GlesRect{rx + 3.0f, ry + 3.0f,
                                  static_cast<float>(cell_ - 6), 3.0f}, slot);
      renderer->FillRect(GlesRect{rx + 3.0f, ry + 3.0f, 3.0f,
                                  static_cast<float>(cell_ - 6)}, slot);
      renderer->SetBlendMode(kGlesBlendOpaque);
      if (tiles_[y][x]) DrawTile(renderer, x, y, tiles_[y][x]);
    }
  }
}

void Game2048::DrawTile(Gles2Renderer* renderer, int col, int row, int value) {
  const arcade::Palette& p = arcade::DefaultPalette();
  const int color_index = TileColorIndex(value);
  const GlesColor base = p.pieces[color_index];
  const float x = static_cast<float>(board_x_ + gap_ + col * (cell_ + gap_));
  const float y = static_cast<float>(board_y_ + gap_ + row * (cell_ + gap_));
  {
    const GlesColor outline = arcade::MixColors(base, p.block_shadow, 0.72f);
    const GlesColor shadow = arcade::MixColors(base, p.block_shadow, 0.10f);
    const GlesColor light = arcade::MixColors(base, p.block_highlight, 0.22f);
    const GlesColor highlight = arcade::MixColors(base, p.block_highlight, 0.72f);
    const GlesColor core = arcade::MixColors(base, p.overlay_text, 0.10f);
    const AsciiSpritePalette sprite = MakeAsciiSpritePalette(
        outline, shadow, base, light, highlight, core, light);
    // The jewel deliberately reaches into most of the board's inter-cell gap:
    // the diamond reads as a full tile rather than a small token in a slot,
    // while the remaining gap keeps adjacent facets visually separate.
    const float gem_bleed = static_cast<float>(gap_) * 0.45f;
    DrawAsciiSprite(renderer, game2048_assets::kTile,
                    GlesRect{x - gem_bleed, y - gem_bleed,
                             static_cast<float>(cell_) + gem_bleed * 2.0f,
                             static_cast<float>(cell_) + gem_bleed * 2.0f},
                    sprite);
    char tile_label[8];
    snprintf(tile_label, sizeof(tile_label), "%d", value);
    int tile_scale = arcade::MaxInt(5, cell_ / 18);
    if (value >= 128) tile_scale = arcade::MaxInt(5, cell_ / 22);
    if (value >= 1024) tile_scale = arcade::MaxInt(4, cell_ / 27);
    const int text_w = arcade::PixelTextWidth(tile_label, tile_scale);
    const int tx = static_cast<int>(x) + (cell_ - text_w) / 2;
    const int ty = static_cast<int>(y) + (cell_ - 7 * tile_scale) / 2;
    const bool bright_tile = arcade::ColorLuma(base) > 0.62f;
    const GlesColor text = bright_tile ? p.overlay_panel : p.overlay_text;
    arcade::DrawPixelText(renderer, tile_label, tx, ty, tile_scale, text);
    return;
  }
  const float inset = arcade::MaxInt(6, cell_ / 14);
  const float edge = arcade::MaxInt(3, cell_ / 30);
  GlesColor glow = base;
  glow.a = 0.20f;
  renderer->SetBlendMode(kGlesBlendAlpha);
  renderer->FillRect(GlesRect{x - edge, y - edge,
                              static_cast<float>(cell_) + edge * 2.0f,
                              static_cast<float>(cell_) + edge * 2.0f}, glow);
  renderer->SetBlendMode(kGlesBlendOpaque);
  renderer->FillRect(GlesRect{x, y, static_cast<float>(cell_),
                              static_cast<float>(cell_)}, base);
  // Dark inset edge plus chunky highlights match the glossy Arkanoid blocks
  // while preserving a clean square silhouette for the 2048 grid.
  GlesColor edge_dark = arcade::MixColors(base, p.block_shadow, 0.62f);
  renderer->FillRect(GlesRect{x, y, static_cast<float>(cell_), edge}, edge_dark);
  renderer->FillRect(GlesRect{x, y, edge, static_cast<float>(cell_)}, edge_dark);
  const float face_pad = arcade::MaxInt(10, cell_ / 9);
  const float face_w = static_cast<float>(cell_) - face_pad * 2.0f;
  const float face_h = face_w;
  GlesColor face = arcade::MixColors(base, p.overlay_text, 0.10f);
  GlesColor face_top = arcade::MixColors(face, p.block_highlight, 0.22f);
  GlesColor face_bottom = arcade::MixColors(face, p.block_shadow, 0.13f);
  renderer->FillRect(GlesRect{x + face_pad, y + face_pad, face_w, face_h}, face);
  renderer->SetBlendMode(kGlesBlendAlpha);
  renderer->FillRect(GlesRect{x + face_pad, y + face_pad, face_w,
                              face_h * 0.32f}, face_top);
  renderer->FillRect(GlesRect{x + face_pad, y + face_pad + face_h * 0.72f,
                              face_w, face_h * 0.28f}, face_bottom);
  renderer->SetBlendMode(kGlesBlendOpaque);
  const float shine_h = arcade::MaxInt(4, cell_ / 19);
  const float shadow_h = arcade::MaxInt(4, cell_ / 17);
  GlesColor shine = p.block_highlight;
  shine.a = 0.78f;
  GlesColor shadow = p.block_shadow;
  shadow.a = 0.48f;
  renderer->SetBlendMode(kGlesBlendAlpha);
  renderer->FillRect(GlesRect{x + inset, y + inset,
                              static_cast<float>(cell_) - inset * 2.0f,
                              shine_h}, shine);
  renderer->FillRect(GlesRect{x + inset, y + inset,
                              shine_h, static_cast<float>(cell_) - inset * 2.0f},
                     shine);
  renderer->FillRect(GlesRect{x + inset,
                              y + static_cast<float>(cell_) - inset -
                                  shadow_h,
                              static_cast<float>(cell_) - inset * 2.0f,
                              shadow_h}, shadow);
  renderer->FillRect(GlesRect{x + static_cast<float>(cell_) - inset - shadow_h,
                              y + inset, shadow_h,
                              static_cast<float>(cell_) - inset * 2.0f}, shadow);
  // Small stepped glints make the tile feel like a lit arcade object rather
  // than a flat swatch, while the broad dark lower band keeps the number open.
  GlesColor glint = p.block_highlight;
  glint.a = 0.90f;
  renderer->SetBlendMode(kGlesBlendAlpha);
  renderer->FillRect(GlesRect{x + face_pad + 4.0f, y + face_pad + 4.0f,
                              static_cast<float>(arcade::MaxInt(8, cell_ / 8)),
                              static_cast<float>(arcade::MaxInt(3, cell_ / 32))},
                     glint);
  renderer->FillRect(GlesRect{x + face_pad + 4.0f, y + face_pad + 4.0f,
                              static_cast<float>(arcade::MaxInt(3, cell_ / 32)),
                              static_cast<float>(arcade::MaxInt(8, cell_ / 8))},
                     glint);
  renderer->SetBlendMode(kGlesBlendOpaque);

  char label[8];
  snprintf(label, sizeof(label), "%d", value);
  int scale = arcade::MaxInt(5, cell_ / 18);
  if (value >= 128) scale = arcade::MaxInt(5, cell_ / 22);
  if (value >= 1024) scale = arcade::MaxInt(4, cell_ / 27);
  int text_w = arcade::PixelTextWidth(label, scale);
  int tx = static_cast<int>(x) + (cell_ - text_w) / 2;
  int ty = static_cast<int>(y) + (cell_ - 7 * scale) / 2;
  const bool bright_tile = arcade::ColorLuma(base) > 0.62f;
  const GlesColor text = bright_tile ? p.overlay_panel : p.overlay_text;
  const GlesColor text_shadow = bright_tile ? p.block_highlight : p.block_shadow;
  const int text_shadow_px = arcade::MaxInt(2, scale / 2);
  arcade::DrawPixelText(renderer, label, tx + text_shadow_px, ty, scale, text_shadow);
  arcade::DrawPixelText(renderer, label, tx - text_shadow_px, ty, scale, text_shadow);
  arcade::DrawPixelText(renderer, label, tx, ty + text_shadow_px, scale, text_shadow);
  arcade::DrawPixelText(renderer, label, tx + text_shadow_px,
                        ty + text_shadow_px, scale,
                        text_shadow);
  arcade::DrawPixelText(renderer, label, tx, ty, scale, text);
}

void Game2048::DrawHud(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::HudLayout layout = arcade::ComputeHudLayout(
      GlesRect{static_cast<float>(hud_x_), static_cast<float>(hud_y_),
               static_cast<float>(hud_w_), static_cast<float>(hud_h_)}, 5);
  arcade::DrawHudPanel(renderer, layout, style);
  arcade::HudFlow flow = arcade::BeginHudFlow(layout);
  arcade::DrawHudNumber(renderer, &flow, layout, "SCORE", score_, 5,
                        p.score, style);
  arcade::DrawHudTextValue(renderer, &flow, layout, "SOUND",
                           sound_muted_ ? "OFF" : "ON", p.score, style);
}

void Game2048::DrawOverlay(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  const char* text = game_over_ ? "GAME OVER" : (win_notice_ ? "YOU WIN" : "PAUSED");
  arcade::DrawCenteredOverlay(
      renderer,
      GlesRect{static_cast<float>(board_x_), static_cast<float>(board_y_),
               static_cast<float>(board_size_), static_cast<float>(board_size_)},
      text, game_over_ ? p.game_over_border :
            (win_notice_ ? p.board_border : p.pause_border),
      arcade::MaxInt(3, board_size_ / 170));
}
