#include "bubble_orbit_game.h"
#include "bubble_orbit_assets.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/color_util.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"

#include <math.h>
#include <string.h>

namespace {
void HexNeighbor(int row, int col, int index, int* out_row, int* out_col) {
  static const int kRow[] = {0, 0, -1, -1, 1, 1};
  static const int kEvenCol[] = {-1, 1, -1, 0, -1, 0};
  static const int kOddCol[] = {-1, 1, 0, 1, 0, 1};
  *out_row = row + kRow[index];
  *out_col = col + ((row & 1) ? kOddCol[index] : kEvenCol[index]);
}

// Levels are built as an eight-layout "orbit".  The first orbit introduces
// distinct shot problems; later orbits revisit them with a deeper ceiling and
// a little more color disruption instead of merely adding random bubbles.
int LevelRows(int level) {
  static const int kBaseRows[] = {4, 5, 5, 6, 6, 7, 7, 8};
  const int orbit = (level - 1) / 8;
  return arcade::ClampInt(kBaseRows[(level - 1) % 8] + (orbit > 0 ? 1 : 0), 4, 8);
}

int LevelColors(int level) {
  if (level <= 2) return 3;
  if (level <= 5) return 4;
  if (level <= 9) return 5;
  return 6;
}

unsigned LevelHash(int level, int row, int col) {
  unsigned value = static_cast<unsigned>(level) * 0x9e3779b9u;
  value ^= static_cast<unsigned>(row + 11) * 0x85ebca6bu;
  value ^= static_cast<unsigned>(col + 17) * 0xc2b2ae35u;
  value ^= value >> 16;
  value *= 0x7feb352du;
  return value ^ (value >> 15);
}

bool LevelCellFilled(int level, int row, int col, int rows) {
  if (row == 0) return true;  // Every generated formation stays anchored.

  const int layout = (level - 1) % 8;
  bool filled = true;
  switch (layout) {
    case 0:  // Canopy: a forgiving, shallow introduction.
      filled = row < rows - 1 || (col >= 2 && col <= 11);
      break;
    case 1:  // Slanted windows teach bank shots into open pockets.
      filled = !((col + row * 2) % 7 == 5 && row >= 2);
      break;
    case 2:  // Twin towers leave a clean central firing lane.
      filled = row < 2 || col <= 4 || col >= 9 ||
               (row == 2 && (col == 5 || col == 8));
      break;
    case 3: {  // Orbit: a diamond-shaped cavity closes toward the ceiling.
      const int center_distance = col > 6 ? col - 7 : 6 - col;
      filled = row < 2 || center_distance + row < 6 ||
               center_distance >= 5;
      break;
    }
    case 4: {  // Wave: alternating column depths create many drop targets.
      static const int kDepth[] = {3, 4, 5, 6, 5, 4, 5,
                                   6, 5, 4, 5, 6, 4, 3};
      filled = row < arcade::ClampInt(kDepth[col], 1, rows);
      break;
    }
    case 5:  // Chambers: small internal gaps reward deliberate color routes.
      filled = !(row >= 2 && ((col + 2 * (row & 1)) % 5 == 2));
      break;
    case 6: {  // Crown: a deep center with two guarded side notches.
      const int center_distance = col > 6 ? col - 7 : 6 - col;
      const int depth = rows - (center_distance >= 5 ? 2 :
                                (center_distance >= 3 ? 1 : 0));
      filled = row < depth && !(row == 2 && (col == 3 || col == 10));
      break;
    }
    default:  // Storm: dense but deterministic end-of-orbit finale.
      filled = row < 2 || LevelHash(level, row, col) % 9u >= 2u;
      break;
  }

  // Familiar layouts become denser when they return in a later orbit.
  const int orbit = (level - 1) / 8;
  if (!filled && orbit > 0) {
    const unsigned refill_chance = static_cast<unsigned>(arcade::ClampInt(orbit, 1, 3));
    filled = LevelHash(level + 31, row, col) % 5u < refill_chance;
  }
  return filled;
}

int LevelCellColor(int level, int row, int col, int colors) {
  const int layout = (level - 1) % 8;
  int group = 0;
  switch (layout) {
    case 0: group = col / 2 + row / 2; break;
    case 1: group = (col + row) / 2; break;
    case 2: group = (col < 7 ? col : 13 - col) / 2 + row; break;
    case 3: group = ((col + row / 2) / 2) + (row > 2 ? 1 : 0); break;
    case 4: group = col / 2 + (row + col / 3) / 2; break;
    case 5: group = (col + (row & 1)) / 2 + row / 2; break;
    case 6: group = (col < 7 ? col : 13 - col) / 2 + row / 2; break;
    default: group = (col + row * 2) / 2; break;
  }

  int color = (group + level - 1) % colors;

  // Keep the broad formations readable, but break up their overly regular
  // two-bubble color bands with a few deterministic neighboring accents.
  // Later levels get gradually livelier without becoming random noise.
  const int orbit = (level - 1) / 8;
  const int accents = arcade::ClampInt(2 + level / 5 + orbit, 2, 6);
  const unsigned variation = LevelHash(level + 73, row, col);
  if (variation % 23u < static_cast<unsigned>(accents)) {
    const int neighbor = ((variation >> 8) & 1u) ? 1 : colors - 1;
    color = (color + neighbor) % colors;
  }
  return color;
}

}

BubbleShooterGame::BubbleShooterGame()
    : width_(1280), height_(720), board_x_(0), board_y_(0), board_w_(0),
      board_h_(0), hud_x_(0), hud_w_(0), radius_(20), aim_angle_(0),
      shot_x_(0), shot_y_(0), shot_vx_(0), shot_vy_(0), row_shift_ms_(0.0),
      level_(1), score_(0), misses_(0), shot_color_(0), next_color_(1),
      speed_(4), shot_active_(false),
      paused_(false), game_over_(false), win_notice_(false), sound_muted_(false),
      sound_events_(0), rng_(0xBABB1Eu) {
  memset(particles_, 0, sizeof(particles_));
  memset(flying_bubbles_, 0, sizeof(flying_bubbles_));
  Reset();
}

void BubbleShooterGame::Reset() {
  level_ = 1;
  score_ = 0;
  speed_ = 4;
  paused_ = game_over_ = win_notice_ = shot_active_ = false;
  sound_events_ = 0;
  aim_angle_ = 0.0f;
  row_shift_ms_ = 0.0;
  memset(bubbles_, -1, sizeof(bubbles_));
  memset(particles_, 0, sizeof(particles_));
  memset(flying_bubbles_, 0, sizeof(flying_bubbles_));
  NewLevel();
}

void BubbleShooterGame::Resize(int width, int height) {
  width_ = width; height_ = height; Layout();
}

void BubbleShooterGame::Layout() {
  int margin = arcade::ScreenMargin(width_, height_);
  hud_w_ = arcade::SideHudWidth(width_);
  int gap = arcade::SideHudGap(width_);
  int available_w = width_ - margin * 2 - hud_w_ - gap;
  int available_h = height_ - margin * 2;
  // Let the playfield use the full space left of the HUD. The old fixed
  // 760x560 cap made the game look like a small card on a living-room TV.
  board_w_ = available_w;
  board_h_ = available_h;
  board_x_ = margin;
  board_y_ = margin;
  hud_x_ = board_x_ + board_w_ + gap;
  const float width_radius = static_cast<float>(board_w_) / 30.0f;
  const float height_radius = static_cast<float>(board_h_ - 18) / 20.0f;
  radius_ = arcade::ClampFloat(width_radius < height_radius ? width_radius : height_radius,
                   12.0f, 34.0f);
}

void BubbleShooterGame::NewLevel() {
  memset(bubbles_, -1, sizeof(bubbles_));
  const int rows = LevelRows(level_);
  const int colors = LevelColors(level_);
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < kCols; ++c) {
      bool filled = LevelCellFilled(level_, r, c, rows);
      if (r > 0) {
        int upper_a_row, upper_a_col, upper_b_row, upper_b_col;
        HexNeighbor(r, c, 2, &upper_a_row, &upper_a_col);
        HexNeighbor(r, c, 3, &upper_b_row, &upper_b_col);
        const bool anchored =
            (InBounds(upper_a_row, upper_a_col) &&
             bubbles_[upper_a_row][upper_a_col] >= 0) ||
            (InBounds(upper_b_row, upper_b_col) &&
             bubbles_[upper_b_row][upper_b_col] >= 0);
        filled = filled && anchored;
      }
      if (filled)
        bubbles_[r][c] = LevelCellColor(level_, r, c, colors);
    }
  }
  misses_ = 0;
  shot_color_ = RandomActiveColor();
  next_color_ = RandomActiveColor();
}

void BubbleShooterGame::Update(double dt_ms, const InputState& input) {
  pixel_effects::UpdateParticles(particles_, kMaxParticles, dt_ms);
  UpdateFlyingBubbles(dt_ms);
  if (game_over_ || win_notice_) {
    if (input.Pressed(kActionPrimary)) { ++level_; NewLevel(); game_over_ = win_notice_ = false; }
    return;
  }
  if (paused_) return;
  if (row_shift_ms_ > 0.0) {
    row_shift_ms_ -= dt_ms;
    if (row_shift_ms_ < 0.0) {
      row_shift_ms_ = 0.0;
      if (DropUnsupportedBubbles() > 0) QueueSound(kSfxBonus);
    }
    return;
  }
  const double repeat_delay_ms = 125.0;
  const double repeat_interval_ms = 48.0;
  const double previous_left = input.HeldMs(kActionLeft) - dt_ms;
  const double previous_right = input.HeldMs(kActionRight) - dt_ms;
  if (input.Pressed(kActionLeft) ||
      (input.Down(kActionLeft) && input.HeldMs(kActionLeft) >= repeat_delay_ms &&
       static_cast<int>((input.HeldMs(kActionLeft) - repeat_delay_ms) / repeat_interval_ms) !=
           static_cast<int>((previous_left - repeat_delay_ms) / repeat_interval_ms)))
    aim_angle_ -= 0.05f;
  if (input.Pressed(kActionRight) ||
      (input.Down(kActionRight) && input.HeldMs(kActionRight) >= repeat_delay_ms &&
       static_cast<int>((input.HeldMs(kActionRight) - repeat_delay_ms) / repeat_interval_ms) !=
           static_cast<int>((previous_right - repeat_delay_ms) / repeat_interval_ms)))
    aim_angle_ += 0.05f;
  aim_angle_ = arcade::ClampFloat(aim_angle_, -1.42f, 1.42f);
  if ((input.Pressed(kActionPrimary) || input.Pressed(kActionUp)) &&
      !shot_active_) Shoot();
  if (!shot_active_) return;
  float dt = static_cast<float>(dt_ms / 1000.0);
  shot_x_ += shot_vx_ * dt;
  shot_y_ += shot_vy_ * dt;
  if (shot_x_ < board_x_ + PlayInset() ||
      shot_x_ > board_x_ + board_w_ - PlayInset()) {
    shot_vx_ = -shot_vx_;
    shot_x_ = arcade::ClampFloat(shot_x_, board_x_ + PlayInset(),
                     board_x_ + board_w_ - PlayInset());
  }
  bool hit = shot_y_ - radius_ * 1.10f <= board_y_ + 6.0f;
  for (int r = 0; r < kRows && !hit; ++r) for (int c = 0; c < kCols; ++c) {
    if (bubbles_[r][c] < 0) continue;
    float dx = shot_x_ - BubbleX(r, c);
    float dy = shot_y_ - BubbleY(r);
    if (dx * dx + dy * dy < radius_ * radius_ * 3.35f) hit = true;
  }
  if (hit) LockShot();
}

void BubbleShooterGame::Shoot() {
  shot_active_ = true;
  shot_x_ = board_x_ + board_w_ * 0.5f;
  shot_y_ = board_y_ + board_h_ - PlayInset();
  const float velocity = 780.0f + speed_ * 55.0f;
  shot_vx_ = sinf(aim_angle_) * velocity;
  shot_vy_ = -cosf(aim_angle_) * velocity;
  QueueSound(kSfxBrick);
}

int BubbleShooterGame::NearestRow(float y) const {
  return arcade::ClampInt(static_cast<int>((y - board_y_ - radius_) / (radius_ * 1.72f) + 0.5f), 0, kRows - 1);
}
int BubbleShooterGame::NearestCol(float x, int row) const {
  float offset = (row & 1) ? radius_ : 0.0f;
  return arcade::ClampInt(static_cast<int>((x - board_x_ - radius_ - offset) / (radius_ * 2.0f) + 0.5f), 0, kCols - 1);
}
float BubbleShooterGame::BubbleX(int row, int col) const {
  const float center = board_x_ + board_w_ * 0.5f;
  const float column = static_cast<float>(col) - (kCols - 1) * 0.5f;
  const float stagger = (row & 1) ? radius_ * 0.5f : -radius_ * 0.5f;
  return center + column * radius_ * 2.0f + stagger;
}
float BubbleShooterGame::BubbleY(int row) const {
  return board_y_ + PlayInset() + row * radius_ * 1.72f;
}
float BubbleShooterGame::PlayInset() const { return radius_ * 1.15f + 6.0f; }
bool BubbleShooterGame::InBounds(int row, int col) const { return row >= 0 && row < kRows && col >= 0 && col < kCols; }
bool BubbleShooterGame::HasBubbleNeighbor(int row, int col) const {
  for (int i = 0; i < 6; ++i) {
    int nr, nc;
    HexNeighbor(row, col, i, &nr, &nc);
    if (InBounds(nr, nc) && bubbles_[nr][nc] >= 0) return true;
  }
  return false;
}
bool BubbleShooterGame::FindAttachCell(float x, float y, int* row,
                                       int* col) const {
  float best_distance = 1e30f;
  bool found = false;
  for (int r = 0; r < kRows; ++r) {
    for (int c = 0; c < kCols; ++c) {
      if (bubbles_[r][c] >= 0 || (r != 0 && !HasBubbleNeighbor(r, c))) continue;
      float dx = x - BubbleX(r, c);
      float dy = y - BubbleY(r);
      float distance = dx * dx + dy * dy;
      if (distance < best_distance) {
        best_distance = distance;
        *row = r;
        *col = c;
        found = true;
      }
    }
  }
  return found;
}

void BubbleShooterGame::LockShot() {
  shot_active_ = false;
  int row = NearestRow(shot_y_), col = NearestCol(shot_x_, row);
  if (!FindAttachCell(shot_x_, shot_y_, &row, &col)) {
    game_over_ = true; QueueSound(kSfxLose); return;
  }
  bubbles_[row][col] = shot_color_;
  bool seen[kRows][kCols] = {};
  int qr[kRows * kCols], qc[kRows * kCols], count = 0, head = 0;
  qr[count] = row; qc[count++] = col; seen[row][col] = true;
  while (head < count) {
    int r = qr[head], c = qc[head++];
    for (int i = 0; i < 6; ++i) { int nr, nc; HexNeighbor(r, c, i, &nr, &nc);
      if (InBounds(nr, nc) && !seen[nr][nc] && bubbles_[nr][nc] == shot_color_) {
        seen[nr][nc] = true; qr[count] = nr; qc[count++] = nc;
      }
    }
  }
  if (count >= 3) {
    for (int i = 0; i < count; ++i) {
      int r = qr[i], c = qc[i];
      SpawnFlyingBubble(BubbleX(r, c), BubbleY(r), bubbles_[r][c], false);
      bubbles_[r][c] = -1;
    }
    misses_ = 0; score_ += count * 10 * level_; QueueSound(kSfxBonus);
    bool attached[kRows][kCols] = {}; count = head = 0;
    for (int c = 0; c < kCols; ++c) if (bubbles_[0][c] >= 0) { attached[0][c] = true; qr[count] = 0; qc[count++] = c; }
    while (head < count) { int r = qr[head], c = qc[head++];
      for (int i=0;i<6;++i) { int nr, nc; HexNeighbor(r,c,i,&nr,&nc); if(InBounds(nr,nc)&&!attached[nr][nc]&&bubbles_[nr][nc]>=0){attached[nr][nc]=true;qr[count]=nr;qc[count++]=nc;} }
    }
    for (int r=0;r<kRows;++r) for(int c=0;c<kCols;++c) {
      if (bubbles_[r][c] >= 0 && !attached[r][c]) {
        SpawnFlyingBubble(BubbleX(r, c), BubbleY(r), bubbles_[r][c], true);
        bubbles_[r][c] = -1;
        score_ += 5;
      }
    }
  } else if (++misses_ >= 3) { misses_ = 0; AddRow(); }
  // The HUD promises the next projectile, so promote that exact color first.
  shot_color_ = next_color_;
  next_color_ = RandomActiveColor();
  bool any = false; for (int r=0;r<kRows;++r) for(int c=0;c<kCols;++c) any |= bubbles_[r][c]>=0;
  if (!any) { win_notice_ = true; QueueSound(kSfxLevel); }
  for (int c=0;c<kCols;++c) if (bubbles_[kRows-2][c]>=0 || bubbles_[kRows-1][c]>=0) game_over_ = true;
}

void BubbleShooterGame::AddRow() {
  for (int r=kRows-1;r>0;--r) for(int c=0;c<kCols;++c) bubbles_[r][c]=bubbles_[r-1][c];
  for (int c=0;c<kCols;++c)
    bubbles_[0][c] = RandomActiveColor();
  row_shift_ms_ = kRowShiftDurationMs;
  QueueSound(kSfxLose);
}

int BubbleShooterGame::RandomActiveColor() {
  bool present[kColors] = {};
  int colors[kColors];
  int count = 0;
  for (int r = 0; r < kRows; ++r)
    for (int c = 0; c < kCols; ++c)
      if (bubbles_[r][c] >= 0) present[bubbles_[r][c]] = true;
  for (int color = 0; color < kColors; ++color)
    if (present[color]) colors[count++] = color;
  return count > 0 ? colors[arcade::NextRandom(&rng_) % count] : 0;
}

int BubbleShooterGame::DropUnsupportedBubbles() {
  bool attached[kRows][kCols] = {};
  int qr[kRows * kCols], qc[kRows * kCols], count = 0, head = 0;
  for (int c = 0; c < kCols; ++c) {
    if (bubbles_[0][c] < 0) continue;
    attached[0][c] = true;
    qr[count] = 0;
    qc[count++] = c;
  }
  while (head < count) {
    const int row = qr[head];
    const int col = qc[head++];
    for (int i = 0; i < 6; ++i) {
      int next_row, next_col;
      HexNeighbor(row, col, i, &next_row, &next_col);
      if (!InBounds(next_row, next_col) || attached[next_row][next_col] ||
          bubbles_[next_row][next_col] < 0) continue;
      attached[next_row][next_col] = true;
      qr[count] = next_row;
      qc[count++] = next_col;
    }
  }
  int dropped = 0;
  for (int row = 0; row < kRows; ++row) for (int col = 0; col < kCols; ++col) {
    if (bubbles_[row][col] < 0 || attached[row][col]) continue;
    SpawnFlyingBubble(BubbleX(row, col), BubbleY(row), bubbles_[row][col], true);
    bubbles_[row][col] = -1;
    ++dropped;
    score_ += 5;
  }
  return dropped;
}

void BubbleShooterGame::SpawnPop(float x, float y, int color, int count) {
  pixel_effects::SpawnParticles(particles_, kMaxParticles, x, y, color,
                                count, &rng_);
}

void BubbleShooterGame::SpawnFlyingBubble(float x, float y, int color,
                                          bool detached) {
  for (int i = 0; i < kMaxFlyingBubbles; ++i) {
    FlyingBubble& bubble = flying_bubbles_[i];
    if (bubble.active) continue;
    const float angle = static_cast<float>(arcade::NextRandom(&rng_) % 628) / 100.0f;
    const float velocity = static_cast<float>(230 + arcade::NextRandom(&rng_) % 250);
    bubble.x = x;
    bubble.y = y;
    bubble.vx = cosf(angle) * velocity;
    bubble.vy = sinf(angle) * velocity - (detached ? 90.0f : 170.0f);
    bubble.life = bubble.max_life =
        static_cast<float>((detached ? 250 : 190) + arcade::NextRandom(&rng_) % 90);
    bubble.color = color;
    bubble.active = true;
    return;
  }
}

void BubbleShooterGame::UpdateFlyingBubbles(double dt_ms) {
  const float dt = static_cast<float>(dt_ms / 1000.0);
  for (int i = 0; i < kMaxFlyingBubbles; ++i) {
    FlyingBubble& bubble = flying_bubbles_[i];
    if (!bubble.active) continue;
    bubble.life -= static_cast<float>(dt_ms);
    bubble.vy += 300.0f * dt;
    bubble.x += bubble.vx * dt;
    bubble.y += bubble.vy * dt;
    if (bubble.life <= 0.0f) {
      SpawnPop(bubble.x, bubble.y, bubble.color, 10);
      bubble.active = false;
    }
  }
}

void BubbleShooterGame::DrawFlyingBubbles(Gles2Renderer* renderer) {
  for (int i = 0; i < kMaxFlyingBubbles; ++i) {
    const FlyingBubble& bubble = flying_bubbles_[i];
    if (!bubble.active) continue;
    const float t = bubble.life / bubble.max_life;
    DrawBubble(renderer, bubble.x, bubble.y, radius_ * (0.78f + t * 0.14f),
               bubble.color);
  }
}

void BubbleShooterGame::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  DrawBackground(renderer);
  DrawBoard(renderer);
  DrawFlyingBubbles(renderer);
  pixel_effects::DrawParticles(renderer, particles_, kMaxParticles,
                               arcade::DefaultPalette().pieces, 7);
  DrawHud(renderer);
  if (paused_ || game_over_ || win_notice_) DrawOverlay(renderer);
}
void BubbleShooterGame::SetPaused(bool paused) { if (!game_over_ && !win_notice_) { paused_=paused; QueueSound(kSfxPause); } }
void BubbleShooterGame::ToggleSoundMute() { sound_muted_ = !sound_muted_; }
void BubbleShooterGame::CycleDifficulty() { speed_ = arcade::CycleInt(speed_, 1, 9); QueueSound(kSfxLevel); }
uint32_t BubbleShooterGame::ConsumeSoundEvents() { return arcade::ConsumeSoundEvents(&sound_events_); }
void BubbleShooterGame::QueueSound(ArcadeSfx sfx) { arcade::QueueSoundEvent(&sound_events_, sfx); }
void BubbleShooterGame::DrawBackground(Gles2Renderer* r) { r->BeginFrame(arcade::DefaultPalette().screen_bg); arcade::DrawBackgroundPattern(r, GlesRect{0,0,(float)width_,(float)height_}, level_, width_/14); }

void BubbleShooterGame::DrawBubble(Gles2Renderer* r, float x, float y, float rad, int color) {
  const arcade::Palette& p = arcade::DefaultPalette();
  GlesColor base = p.pieces[color % kColors];
  AsciiSpritePalette sprite = MakeAsciiSpritePalette(
      arcade::ScaleRgb(base, 0.42f), arcade::ScaleRgb(base, 0.68f), base, arcade::ScaleRgb(base, 1.25f),
      arcade::Rgba(255,255,255,255), arcade::ScaleRgb(base, 1.55f),
      arcade::ScaleRgb(base, 1.45f));
  sprite.inks[kAsciiSoftGlow].a = 0.12f;
  sprite.inks[kAsciiGlow].a = 0.30f;
  sprite.inks[kAsciiHotGlow].a = 0.90f;
  DrawAsciiSprite(r, bubble_assets::kBubble,
                  GlesRect{x-rad, y-rad, rad*2.0f, rad*2.0f}, sprite);
  r->SetBlendMode(kGlesBlendOpaque);
}
void BubbleShooterGame::DrawBoard(Gles2Renderer* r) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawPanel(
      r, GlesRect{static_cast<float>(board_x_), static_cast<float>(board_y_),
                  static_cast<float>(board_w_), static_cast<float>(board_h_)},
      p.playfield_bg, p.board_border,
      static_cast<float>(arcade::PanelBorderSize(hud_w_)));
  const float progress = row_shift_ms_ > 0.0 ?
      1.0f - static_cast<float>(row_shift_ms_ / kRowShiftDurationMs) : 1.0f;
  const float eased = progress * progress * (3.0f - 2.0f * progress);
  for(int row=0;row<kRows;++row) for(int col=0;col<kCols;++col) {
    if (bubbles_[row][col] < 0) continue;
    float x = BubbleX(row, col);
    float y = BubbleY(row);
    if (row_shift_ms_ > 0.0) {
      const float from_x = BubbleX(row - 1, col);
      const float from_y = BubbleY(row - 1);
      x = from_x + (x - from_x) * eased;
      y = from_y + (y - from_y) * eased;
    }
    DrawBubble(r, x, y, radius_-2, bubbles_[row][col]);
  }
  r->SetBlendMode(kGlesBlendAlpha); GlesColor guide=p.label; guide.a=0.58f; float sx=board_x_+board_w_*0.5f, sy=board_y_+board_h_-PlayInset(); for(int i=1;i<18;++i) r->FillRect(GlesRect{sx+sinf(aim_angle_)*i*12-2,sy-cosf(aim_angle_)*i*12-2,4,4},guide); r->SetBlendMode(kGlesBlendOpaque);
  if (shot_active_) DrawBubble(r,shot_x_,shot_y_,radius_-2,shot_color_); else DrawBubble(r,sx,sy,radius_-2,shot_color_);
}
void BubbleShooterGame::DrawHud(Gles2Renderer* r) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::HudStyle s = arcade::DefaultHudStyle();
  arcade::HudLayout l = arcade::ComputeHudLayout(
      GlesRect{(float)hud_x_, (float)board_y_, (float)hud_w_,
               (float)board_h_}, 5);
  arcade::DrawHudPanel(r, l, s);
  arcade::HudFlow f = arcade::BeginHudFlow(l);
  arcade::DrawHudNumber(r, &f, l, "LEVEL", level_, 2, p.speed, s);
  arcade::DrawHudNumber(r, &f, l, "SCORE", score_, 5, p.score, s);
  arcade::DrawHudNumber(r, &f, l, "SPEED", speed_, 1, p.speed, s);
  arcade::DrawHudNumber(r, &f, l, "MISSES", misses_, 1,
                        p.game_over_border, s);
  int next_y = arcade::ReserveHudBlock(&f, static_cast<int>(radius_ * 2.6f));
  arcade::DrawPixelText(r, "NEXT", l.content_x, next_y, l.label_scale,
                        p.label);
  DrawBubble(r, l.content_x + radius_,
             next_y + l.label_scale * 14 + radius_, radius_ * 0.70f,
             next_color_);
}
void BubbleShooterGame::DrawOverlay(Gles2Renderer* r) { const arcade::Palette&p=arcade::DefaultPalette(); arcade::DrawCenteredOverlay(r,GlesRect{(float)board_x_,(float)board_y_,(float)board_w_,(float)board_h_},game_over_?"GAME OVER":(win_notice_?"YOU WIN":"PAUSED"),game_over_?p.game_over_border:(win_notice_?p.board_border:p.pause_border),3); }
