#include "dragoban_game.h"

#include "engine/src/arcade_hud.h"
#include "engine/src/arcade_style.h"
#include "engine/src/math_util.h"
#include "engine/src/random_util.h"

#include <string.h>

namespace {
// The generated table contains David W. Skinner's complete Microban I set in
// its original order. See ../LEVELS.md and ../../THIRD_PARTY_NOTICES.md for
// provenance and redistribution terms.
GlesColor Color(float r, float g, float b, float a = 1.0f) {
  return GlesColor{r, g, b, a};
}

#include "microban_levels.h"
}  // namespace

SokobanGame::SokobanGame()
    : width_(1280),
      height_(720),
      board_x_(0),
      board_y_(0),
      board_w_(0),
      board_h_(0),
      cell_(0),
      hud_x_(0),
      hud_y_(0),
      hud_w_(0),
      hud_h_(0),
      level_index_(1),
      difficulty_(1),
      moves_(0),
      pushes_(0),
      paused_(false),
      solved_notice_(false),
      hatching_(false),
      solved_timer_ms_(0.0),
      hatch_timer_ms_(0.0),
      wall_hit_ms_(0.0),
      move_flash_ms_(0.0),
      ambient_ms_(0.0),
      wall_hit_(Cell{0, 0}),
      sound_muted_(false),
      sound_events_(0),
      rng_(0x50c0b00u) {
  memset(&level_, 0, sizeof(level_));
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  Reset();
}

void SokobanGame::Reset() {
  moves_ = 0;
  pushes_ = 0;
  paused_ = false;
  solved_notice_ = false;
  hatching_ = false;
  solved_timer_ms_ = 0.0;
  hatch_timer_ms_ = 0.0;
  wall_hit_ms_ = 0.0;
  move_flash_ms_ = 0.0;
  ambient_ms_ = 0.0;
  sound_events_ = 0;
  memset(particles_, 0, sizeof(particles_));
  memset(bursts_, 0, sizeof(bursts_));
  memset(dragons_, 0, sizeof(dragons_));
  GenerateLevel();
}

void SokobanGame::Resize(int width, int height) {
  width_ = width;
  height_ = height;
  Layout();
}

void SokobanGame::Update(double dt_ms, const InputState& input) {
  pixel_effects::UpdateParticles(particles_, kMaxParticles, dt_ms);
  pixel_effects::UpdateBursts(bursts_, kMaxBursts, dt_ms);
  ambient_ms_ += dt_ms;
  if (wall_hit_ms_ > 0.0) {
    wall_hit_ms_ -= dt_ms;
    if (wall_hit_ms_ < 0.0) wall_hit_ms_ = 0.0;
  }
  if (move_flash_ms_ > 0.0) {
    move_flash_ms_ -= dt_ms;
    if (move_flash_ms_ < 0.0) move_flash_ms_ = 0.0;
  }
  if (hatching_) {
    UpdateHatching(dt_ms);
    return;
  }
  if (solved_notice_) {
    solved_timer_ms_ += dt_ms;
    if (solved_timer_ms_ >= 520.0 || input.Pressed(kActionPrimary) ||
        input.Pressed(kActionLeft) ||
        input.Pressed(kActionRight) || input.Pressed(kActionUp) ||
        input.Pressed(kActionDown)) {
      ++level_index_;
      GenerateLevel();
    }
    return;
  }
  if (paused_) return;
  if (input.Pressed(kActionPrimary)) {
    GenerateLevel();
    QueueSound(kSfxPause);
    return;
  }
  if (input.Pressed(kActionLeft)) TryMove(-1, 0);
  else if (input.Pressed(kActionRight)) TryMove(1, 0);
  else if (input.Pressed(kActionUp)) TryMove(0, -1);
  else if (input.Pressed(kActionDown)) TryMove(0, 1);
}

void SokobanGame::Render(Gles2Renderer* renderer) {
  if (!renderer) return;
  DrawBackground(renderer);
  DrawBoard(renderer);
  DrawHud(renderer);
  pixel_effects::DrawParticles(renderer, particles_, kMaxParticles,
                               arcade::DefaultPalette().pieces, 7);
  pixel_effects::DrawBursts(renderer, bursts_, kMaxBursts,
                            arcade::DefaultPalette().pieces, 7,
                            static_cast<float>(cell_ * 2), 3.0f);
  if (paused_ || solved_notice_) DrawOverlay(renderer);
}

void SokobanGame::SetPaused(bool paused) {
  if (solved_notice_ || paused_ == paused) return;
  paused_ = paused;
  QueueSound(kSfxPause);
}

void SokobanGame::ToggleSoundMute() { sound_muted_ = !sound_muted_; }

void SokobanGame::CycleDifficulty() {
  difficulty_ = arcade::CycleInt(difficulty_, 1, 9);
  GenerateLevel();
  QueueSound(kSfxLevel);
}

uint32_t SokobanGame::ConsumeSoundEvents() {
  return arcade::ConsumeSoundEvents(&sound_events_);
}

void SokobanGame::Layout() {
  int min_x = 0;
  int min_y = 0;
  int max_x = kCols - 1;
  int max_y = kRows - 1;
  ActiveBounds(&min_x, &min_y, &max_x, &max_y);
  const int active_cols = arcade::MaxInt(1, max_x - min_x + 1);
  const int active_rows = arcade::MaxInt(1, max_y - min_y + 1);
  const arcade::GridSideLayout layout = arcade::ComputeGridSideLayout(
      width_, height_, active_cols, active_rows, 8, false);
  cell_ = layout.cell;
  board_x_ = layout.play_x; board_y_ = layout.play_y;
  board_w_ = layout.play_w; board_h_ = layout.play_h;
  hud_x_ = layout.hud_x; hud_y_ = layout.hud_y;
  hud_w_ = layout.hud_w; hud_h_ = layout.hud_h;
}

void SokobanGame::GenerateLevel() {
  if (level_index_ < 1 || level_index_ > kCuratedLevelCount) level_index_ = 1;
  Level curated;
  if (!LoadCuratedLevel(&curated, level_index_ - 1) ||
      GoalsSatisfied(curated)) {
    level_index_ = 1;
    LoadCuratedLevel(&curated, 0);
  }
  level_ = curated;
  CenterLevel(&level_);
  moves_ = 0;
  pushes_ = 0;
  solved_notice_ = false;
  hatching_ = false;
  solved_timer_ms_ = 0.0;
  hatch_timer_ms_ = 0.0;
  memset(dragons_, 0, sizeof(dragons_));
  Layout();
}

void SokobanGame::CenterLevel(Level* level) const {
  if (!level) return;
  int min_x = kCols;
  int min_y = kRows;
  int max_x = -1;
  int max_y = -1;
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (!level->walls[y][x]) continue;
      min_x = arcade::MinInt(min_x, x);
      min_y = arcade::MinInt(min_y, y);
      max_x = arcade::MaxInt(max_x, x);
      max_y = arcade::MaxInt(max_y, y);
    }
  }
  for (int i = 0; i < level->box_count; ++i) {
    min_x = arcade::MinInt(min_x, level->boxes[i].x);
    min_y = arcade::MinInt(min_y, level->boxes[i].y);
    max_x = arcade::MaxInt(max_x, level->boxes[i].x);
    max_y = arcade::MaxInt(max_y, level->boxes[i].y);
    min_x = arcade::MinInt(min_x, level->goals[i].x);
    min_y = arcade::MinInt(min_y, level->goals[i].y);
    max_x = arcade::MaxInt(max_x, level->goals[i].x);
    max_y = arcade::MaxInt(max_y, level->goals[i].y);
  }
  min_x = arcade::MinInt(min_x, level->player.x);
  min_y = arcade::MinInt(min_y, level->player.y);
  max_x = arcade::MaxInt(max_x, level->player.x);
  max_y = arcade::MaxInt(max_y, level->player.y);
  if (max_x < min_x || max_y < min_y) return;
  int dx = (kCols - (max_x - min_x + 1) + 1) / 2 - min_x;
  int dy = (kRows - (max_y - min_y + 1) + 1) / 2 - min_y;
  if (dx == 0 && dy == 0) return;
  uint8_t shifted[kRows][kCols];
  memset(shifted, 0, sizeof(shifted));
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (!level->walls[y][x]) continue;
      int nx = x + dx;
      int ny = y + dy;
      if (nx >= 0 && nx < kCols && ny >= 0 && ny < kRows) shifted[ny][nx] = 1;
    }
  }
  memcpy(level->walls, shifted, sizeof(shifted));
  level->player.x += dx;
  level->player.y += dy;
  for (int i = 0; i < level->box_count; ++i) {
    level->boxes[i].x += dx;
    level->boxes[i].y += dy;
    level->goals[i].x += dx;
    level->goals[i].y += dy;
  }
}

bool SokobanGame::LoadCuratedLevel(Level* level, int index) const {
  if (!level || kCuratedLevelCount <= 0) return false;
  memset(level, 0, sizeof(*level));
  const char* const* rows = kCuratedLevels[index % kCuratedLevelCount];
  int boxes = 0;
  int goals = 0;
  bool has_player = false;
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      char ch = rows[y][x];
      if (ch == '#') {
        level->walls[y][x] = 1;
      } else if (ch == '@') {
        level->player = Cell{x, y};
        has_player = true;
      } else if (ch == '+') {
        level->player = Cell{x, y};
        has_player = true;
        if (goals < kMaxBoxes) level->goals[goals++] = Cell{x, y};
      } else if (ch == '$') {
        if (boxes < kMaxBoxes) level->boxes[boxes++] = Cell{x, y};
      } else if (ch == '*') {
        if (boxes < kMaxBoxes) level->boxes[boxes++] = Cell{x, y};
        if (goals < kMaxBoxes) level->goals[goals++] = Cell{x, y};
      } else if (ch == '.') {
        if (goals < kMaxBoxes) level->goals[goals++] = Cell{x, y};
      }
    }
  }
  if (!has_player || boxes <= 0 || goals != boxes || boxes > kMaxBoxes) {
    return false;
  }
  level->box_count = boxes;
  return true;
}

bool SokobanGame::VerifySolvable(const Level& start) const {
  struct State {
    Cell player;
    Cell boxes[kMaxBoxes];
  };
  static const int kMaxStates = 12000;
  State queue[kMaxStates];
  uint32_t hashes[kMaxStates];
  int head = 0;
  int tail = 0;
  queue[tail] = State{start.player, {Cell{0, 0}, Cell{0, 0}, Cell{0, 0},
                                     Cell{0, 0}, Cell{0, 0}}};
  for (int i = 0; i < start.box_count; ++i) queue[tail].boxes[i] = start.boxes[i];
  hashes[tail] = 0;
  ++tail;
  static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  while (head < tail) {
    State s = queue[head++];
    Level level = start;
    level.player = s.player;
    for (int i = 0; i < start.box_count; ++i) level.boxes[i] = s.boxes[i];
    if (GoalsSatisfied(level)) return true;
    for (int d = 0; d < 4; ++d) {
      State n = s;
      int nx = s.player.x + dirs[d][0];
      int ny = s.player.y + dirs[d][1];
      if (nx < 0 || nx >= kCols || ny < 0 || ny >= kRows ||
          start.walls[ny][nx]) continue;
      int bi = BoxAt(level, nx, ny);
      if (bi >= 0) {
        int bx = nx + dirs[d][0];
        int by = ny + dirs[d][1];
        if (bx < 0 || bx >= kCols || by < 0 || by >= kRows ||
            start.walls[by][bx] || BoxAt(level, bx, by) >= 0) {
          continue;
        }
        n.boxes[bi] = Cell{bx, by};
      }
      n.player = Cell{nx, ny};
      uint32_t h = static_cast<uint32_t>(n.player.x + n.player.y * kCols);
      for (int i = 0; i < start.box_count; ++i)
        h = h * 131u + static_cast<uint32_t>(n.boxes[i].x + n.boxes[i].y * kCols);
      bool seen = false;
      for (int i = 0; i < tail; ++i) {
        if (hashes[i] == h) {
          seen = true;
          break;
        }
      }
      if (seen || tail >= kMaxStates) continue;
      hashes[tail] = h;
      queue[tail++] = n;
    }
  }
  return false;
}

bool SokobanGame::TryMove(int dx, int dy) {
  int nx = level_.player.x + dx;
  int ny = level_.player.y + dy;
  if (IsWall(nx, ny)) {
    SpawnWallHit(nx, ny);
    QueueSound(kSfxPaddle);
    return false;
  }
  int bi = BoxAt(nx, ny);
  if (bi >= 0) {
    int bx = nx + dx;
    int by = ny + dy;
    if (IsWall(bx, by) || BoxAt(bx, by) >= 0) {
      SpawnWallHit(bx, by);
      QueueSound(kSfxPaddle);
      return false;
    }
    level_.boxes[bi] = Cell{bx, by};
    ++pushes_;
    QueueSound(kSfxBrick);
  } else {
    QueueSound(kSfxPaddle);
  }
  level_.player = Cell{nx, ny};
  move_flash_ms_ = 120.0;
  ++moves_;
  if (Solved()) {
    BeginHatching();
  }
  return true;
}

bool SokobanGame::Solved() const { return GoalsSatisfied(level_); }

bool SokobanGame::IsWall(int x, int y) const {
  return x < 0 || x >= kCols || y < 0 || y >= kRows || level_.walls[y][x];
}

int SokobanGame::BoxAt(int x, int y) const { return BoxAt(level_, x, y); }

bool SokobanGame::GoalAt(int x, int y) const {
  for (int i = 0; i < level_.box_count; ++i)
    if (level_.goals[i].x == x && level_.goals[i].y == y) return true;
  return false;
}

bool SokobanGame::CellFree(const Level& level, int x, int y) const {
  return x >= 0 && x < kCols && y >= 0 && y < kRows && !level.walls[y][x] &&
         BoxAt(level, x, y) < 0;
}

int SokobanGame::BoxAt(const Level& level, int x, int y) const {
  for (int i = 0; i < level.box_count; ++i)
    if (level.boxes[i].x == x && level.boxes[i].y == y) return i;
  return -1;
}

bool SokobanGame::GoalsSatisfied(const Level& level) const {
  for (int i = 0; i < level.box_count; ++i) {
    bool ok = false;
    for (int j = 0; j < level.box_count; ++j)
      if (level.boxes[i].x == level.goals[j].x &&
          level.boxes[i].y == level.goals[j].y) {
        ok = true;
      }
    if (!ok) return false;
  }
  return true;
}

void SokobanGame::QueueSound(ArcadeSfx sfx) {
  arcade::QueueSoundEvent(&sound_events_, sfx);
}

void SokobanGame::SpawnWallHit(int x, int y) {
  if (x < 0) x = 0;
  if (x >= kCols) x = kCols - 1;
  if (y < 0) y = 0;
  if (y >= kRows) y = kRows - 1;
  wall_hit_ = Cell{x, y};
  wall_hit_ms_ = 150.0;
  pixel_effects::SpawnParticles(particles_, kMaxParticles,
                                static_cast<int>(TileRect(x, y).x +
                                                 TileRect(x, y).w / 2.0f),
                                static_cast<int>(TileRect(x, y).y +
                                                 TileRect(x, y).h / 2.0f),
                                6, 18, &rng_);
}

void SokobanGame::BeginHatching() {
  hatching_ = true;
  hatch_timer_ms_ = 0.0;
  QueueSound(kSfxLevel);
  for (int i = 0; i < level_.box_count; ++i) {
    HatchingDragon& dragon = dragons_[i];
    GlesRect tile = TileRect(level_.boxes[i].x, level_.boxes[i].y);
    dragon.active = true;
    dragon.x = tile.x + tile.w * 0.5f;
    dragon.y = tile.y + tile.h * 0.5f;
    dragon.vx = static_cast<float>((i % 3 - 1) * 145 +
                                   static_cast<int>(arcade::NextRandom(&rng_) % 91) - 45);
    dragon.vy = -static_cast<float>(185 + arcade::NextRandom(&rng_) % 120);
    pixel_effects::SpawnParticles(particles_, kMaxParticles, dragon.x, dragon.y,
                                  9, 26, &rng_);
  }
}

void SokobanGame::UpdateHatching(double dt_ms) {
  hatch_timer_ms_ += dt_ms;
  const float dt = static_cast<float>(dt_ms / 1000.0);
  for (int i = 0; i < level_.box_count; ++i) {
    HatchingDragon& dragon = dragons_[i];
    if (!dragon.active) continue;
    dragon.x += dragon.vx * dt;
    dragon.y += dragon.vy * dt;
    dragon.vy -= 45.0f * dt;
  }
  if (hatch_timer_ms_ >= 1250.0) {
    hatching_ = false;
    solved_notice_ = true;
    solved_timer_ms_ = 0.0;
  }
}

void SokobanGame::DrawBackground(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  renderer->BeginFrame(p.screen_bg);
  renderer->FillRect(GlesRect{0, 0, static_cast<float>(width_),
                              static_cast<float>(height_)}, p.screen_bg);
  DrawPatternPanel(renderer, 0, 0, board_x_, height_, level_index_ - 1);
  int right_x = hud_x_ + hud_w_;
  DrawPatternPanel(renderer, right_x, 0, width_ - right_x, height_,
                   level_index_ - 1);
}

void SokobanGame::DrawPatternPanel(Gles2Renderer* renderer, int x, int y,
                                   int w, int h, int variant) {
  if (w <= 0 || h <= 0) return;
  struct Pattern {
    GlesColor bg;
    GlesColor primary;
    GlesColor secondary;
    GlesColor accent;
    int motif;
    int step_extra;
  };
  const Pattern patterns[] = {
      {{0.043f, 0.063f, 0.098f, 1.0f}, {0.306f, 0.412f, 0.475f, 1.0f},
       {0.671f, 0.545f, 0.459f, 1.0f}, {0.373f, 0.545f, 0.482f, 1.0f}, 0, 0},
      {{0.051f, 0.059f, 0.110f, 1.0f}, {0.373f, 0.365f, 0.518f, 1.0f},
       {0.710f, 0.537f, 0.616f, 1.0f}, {0.467f, 0.592f, 0.557f, 1.0f}, 1, 18},
      {{0.047f, 0.071f, 0.090f, 1.0f}, {0.337f, 0.494f, 0.408f, 1.0f},
       {0.745f, 0.620f, 0.396f, 1.0f}, {0.439f, 0.467f, 0.620f, 1.0f}, 2, 6},
      {{0.067f, 0.055f, 0.110f, 1.0f}, {0.463f, 0.369f, 0.502f, 1.0f},
       {0.569f, 0.620f, 0.478f, 1.0f}, {0.745f, 0.510f, 0.408f, 1.0f}, 3, 28},
      {{0.039f, 0.075f, 0.106f, 1.0f}, {0.294f, 0.486f, 0.541f, 1.0f},
       {0.561f, 0.427f, 0.580f, 1.0f}, {0.690f, 0.620f, 0.463f, 1.0f}, 4, 14},
      {{0.071f, 0.063f, 0.094f, 1.0f}, {0.467f, 0.412f, 0.345f, 1.0f},
       {0.373f, 0.553f, 0.518f, 1.0f}, {0.580f, 0.506f, 0.667f, 1.0f}, 5, 34},
  };
  Pattern ptn = patterns[variant % (sizeof(patterns) / sizeof(patterns[0]))];
  renderer->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(y),
                              static_cast<float>(w), static_cast<float>(h)},
                     ptn.bg);
  int step = arcade::MaxInt(86, cell_ * 2 + ptn.step_extra);
  for (int yy = y - step; yy < y + h + step; yy += step) {
    for (int xx = x - step; xx < x + w + step; xx += step) {
      int gx = (xx - x) / step;
      int gy = (yy - y) / step;
      int selector = (gx * 3 + gy * 5 + variant) & 3;
      if (ptn.motif == 0) {
        renderer->FillRect(GlesRect{static_cast<float>(xx + step / 5),
                                    static_cast<float>(yy + step / 5),
                                    static_cast<float>(step * 3 / 5),
                                    static_cast<float>(step / 8)},
                           selector & 1 ? ptn.primary : ptn.secondary);
        renderer->FillRect(GlesRect{static_cast<float>(xx + step / 5),
                                    static_cast<float>(yy + step / 5),
                                    static_cast<float>(step / 8),
                                    static_cast<float>(step * 3 / 5)},
                           selector & 1 ? ptn.primary : ptn.secondary);
      } else if (ptn.motif == 1) {
        arcade::DrawPanel(renderer,
                          GlesRect{static_cast<float>(xx + step / 5),
                                   static_cast<float>(yy + step / 5),
                                   static_cast<float>(step * 3 / 5),
                                   static_cast<float>(step * 3 / 5)},
                          ptn.bg, selector & 1 ? ptn.accent : ptn.primary,
                          static_cast<float>(arcade::MaxInt(4, step / 14)));
      } else if (ptn.motif == 2) {
        renderer->FillRect(GlesRect{static_cast<float>(xx + step / 3),
                                    static_cast<float>(yy + step / 3),
                                    static_cast<float>(step / 3),
                                    static_cast<float>(step / 3)},
                           ptn.primary);
        renderer->FillRect(GlesRect{static_cast<float>(xx + step * 2 / 3),
                                    static_cast<float>(yy + step / 5),
                                    static_cast<float>(step / 6),
                                    static_cast<float>(step / 6)},
                           ptn.accent);
      } else {
        renderer->FillRect(GlesRect{static_cast<float>(xx + step / 4),
                                    static_cast<float>(yy + step / 2),
                                    static_cast<float>(step / 2),
                                    static_cast<float>(arcade::MaxInt(8, step / 9))},
                           ptn.secondary);
        renderer->FillRect(GlesRect{static_cast<float>(xx + step / 2),
                                    static_cast<float>(yy + step / 4),
                                    static_cast<float>(arcade::MaxInt(8, step / 9)),
                                    static_cast<float>(step / 2)},
                           ptn.primary);
      }
    }
  }
}

void SokobanGame::DrawBoard(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawPanel(renderer, GlesRect{static_cast<float>(board_x_),
                                      static_cast<float>(board_y_),
                                      static_cast<float>(board_w_),
                                      static_cast<float>(board_h_)},
                    p.playfield_bg, p.board_border,
                    static_cast<float>(arcade::PanelBorderSize(hud_w_)));
  uint8_t exterior[kRows][kCols];
  memset(exterior, 0, sizeof(exterior));
  int qx[kRows * kCols];
  int qy[kRows * kCols];
  int head = 0;
  int tail = 0;
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (x != 0 && x != kCols - 1 && y != 0 && y != kRows - 1) continue;
      if (level_.walls[y][x] || exterior[y][x]) continue;
      exterior[y][x] = 1;
      qx[tail] = x;
      qy[tail] = y;
      ++tail;
    }
  }
  static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  while (head < tail) {
    int x = qx[head];
    int y = qy[head];
    ++head;
    for (int d = 0; d < 4; ++d) {
      int nx = x + dirs[d][0];
      int ny = y + dirs[d][1];
      if (nx < 0 || nx >= kCols || ny < 0 || ny >= kRows) continue;
      if (level_.walls[ny][nx] || exterior[ny][nx]) continue;
      exterior[ny][nx] = 1;
      qx[tail] = nx;
      qy[tail] = ny;
      ++tail;
    }
  }
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (level_.walls[y][x]) {
        DrawWallTile(renderer, x, y,
                     wall_hit_ms_ > 0.0 && wall_hit_.x == x && wall_hit_.y == y);
      } else {
        if (exterior[y][x]) continue;
        const bool has_fire = GoalAt(x, y);
        DrawFloorTile(renderer, x, y, has_fire);
        if (has_fire) DrawGoal(renderer, x, y);
      }
    }
  }
  if (!hatching_) {
    for (int i = 0; i < level_.box_count; ++i) {
      Cell b = level_.boxes[i];
      DrawCrate(renderer, b.x, b.y, GoalAt(b.x, b.y));
    }
  }
  DrawRobot(renderer, level_.player.x, level_.player.y);
  if (hatching_)
    for (int i = 0; i < level_.box_count; ++i)
      if (dragons_[i].active) DrawDragon(renderer, dragons_[i]);
}

void SokobanGame::DrawFloorTile(Gles2Renderer* renderer, int x, int y,
                                bool has_fire) {
  GlesRect tile = TileRect(x, y);
  const float u = tile.w / 16.0f;
  const int variant = (x * 17 + y * 31 + level_index_ * 11) % 9;
  const GlesColor rock = Color(0.071f, 0.063f, 0.102f);
  const GlesColor rock_mid = Color(0.114f, 0.090f, 0.137f);
  const GlesColor crevice = Color(0.027f, 0.020f, 0.043f);
  const GlesColor ember = Color(0.922f, 0.220f, 0.035f);
  const GlesColor flame = Color(1.000f, 0.631f, 0.055f);
  renderer->SetBlendMode(kGlesBlendOpaque);
  renderer->FillRect(tile, crevice);
  renderer->FillRect(GlesRect{tile.x + u, tile.y + u, tile.w - 2.0f * u,
                              tile.h - 2.0f * u}, rock);
  renderer->FillRect(GlesRect{tile.x + 2.0f * u, tile.y + 2.0f * u,
                              8.0f * u, 3.0f * u}, rock_mid);
  renderer->FillRect(GlesRect{tile.x + 7.0f * u, tile.y + 8.0f * u,
                              6.0f * u, 4.0f * u}, rock_mid);
  if (!has_fire && (variant == 1 || variant == 4 || variant == 7)) {
    renderer->FillRect(GlesRect{tile.x + 3.0f * u, tile.y + 9.0f * u,
                                7.0f * u, u}, ember);
    renderer->FillRect(GlesRect{tile.x + 8.0f * u, tile.y + 5.0f * u,
                                u, 5.0f * u}, ember);
    renderer->FillRect(GlesRect{tile.x + 8.0f * u, tile.y + 8.0f * u,
                                3.0f * u, u}, flame);
  } else if (!has_fire && (variant == 2 || variant == 6)) {
    renderer->FillRect(GlesRect{tile.x + 4.0f * u, tile.y + 5.0f * u,
                                u, 6.0f * u}, ember);
    renderer->FillRect(GlesRect{tile.x + 4.0f * u, tile.y + 10.0f * u,
                                6.0f * u, u}, flame);
  }
}

void SokobanGame::DrawWallTile(Gles2Renderer* renderer, int x, int y,
                               bool hit) {
  GlesRect tile = TileRect(x, y);
  const float u = tile.w / 16.0f;
  const GlesColor abyss = Color(0.020f, 0.016f, 0.031f);
  const GlesColor stone = hit ? Color(0.525f, 0.161f, 0.129f) :
                                Color(0.173f, 0.122f, 0.216f);
  const GlesColor stone_light = hit ? Color(0.929f, 0.353f, 0.129f) :
                                      Color(0.294f, 0.212f, 0.369f);
  const GlesColor stone_dark = Color(0.078f, 0.047f, 0.114f);
  const GlesColor lava = Color(0.949f, 0.184f, 0.027f);
  renderer->SetBlendMode(kGlesBlendOpaque);
  renderer->FillRect(tile, abyss);
  renderer->FillRect(GlesRect{tile.x + u, tile.y + 2.0f * u, 14.0f * u,
                              12.0f * u}, stone_dark);
  renderer->FillRect(GlesRect{tile.x + 2.0f * u, tile.y + u, 10.0f * u,
                              14.0f * u}, stone);
  renderer->FillRect(GlesRect{tile.x + 12.0f * u, tile.y + 4.0f * u,
                              2.0f * u, 8.0f * u}, stone);
  renderer->FillRect(GlesRect{tile.x + 3.0f * u, tile.y + 2.0f * u,
                              6.0f * u, 2.0f * u}, stone_light);
  renderer->FillRect(GlesRect{tile.x + 2.0f * u, tile.y + 8.0f * u,
                              4.0f * u, 4.0f * u}, stone_light);
  if (((x * 3 + y * 5) & 3) == 0) {
    renderer->FillRect(GlesRect{tile.x + 9.0f * u, tile.y + 5.0f * u,
                                u, 5.0f * u}, lava);
    renderer->FillRect(GlesRect{tile.x + 9.0f * u, tile.y + 9.0f * u,
                                3.0f * u, u}, lava);
  }
}

void SokobanGame::DrawGoal(Gles2Renderer* renderer, int x, int y) {
  GlesRect tile = TileRect(x, y);
  const float u = tile.w / 16.0f;
  const bool flicker = (static_cast<int>(ambient_ms_ / 150.0) + x * 3 + y) & 1;
  const GlesColor coal = Color(0.157f, 0.043f, 0.024f);
  const GlesColor ember = Color(0.949f, 0.176f, 0.020f);
  const GlesColor flame = Color(1.000f, 0.620f, 0.039f);
  renderer->SetBlendMode(kGlesBlendOpaque);
  renderer->FillRect(GlesRect{tile.x + 4.0f * u, tile.y + 11.0f * u,
                              8.0f * u, 2.0f * u}, coal);
  renderer->FillRect(GlesRect{tile.x + 6.0f * u, tile.y + 9.0f * u,
                              4.0f * u, 3.0f * u}, ember);
  renderer->FillRect(GlesRect{tile.x + (flicker ? 7.0f : 6.0f) * u,
                              tile.y + (flicker ? 7.0f : 8.0f) * u,
                              2.0f * u, 3.0f * u}, flame);
  renderer->FillRect(GlesRect{tile.x + (flicker ? 9.0f : 6.0f) * u,
                              tile.y + (flicker ? 8.0f : 7.0f) * u,
                              u, 2.0f * u}, ember);
}

void SokobanGame::DrawCrate(Gles2Renderer* renderer, int x, int y,
                            bool stored) {
  GlesRect tile = TileRect(x, y, 0.5f);
  const float s = tile.w < tile.h ? tile.w : tile.h;
  const float rx = tile.x;
  const float ry = tile.y;
  const float u = s / 20.0f;
  const GlesColor shadow = Color(0.035f, 0.012f, 0.063f);
  const GlesColor outline = stored ? Color(0.220f, 0.055f, 0.078f) :
                                    Color(0.082f, 0.027f, 0.149f);
  const GlesColor shell = stored ? Color(0.945f, 0.176f, 0.047f) :
                                   Color(0.024f, 0.510f, 0.643f);
  const GlesColor core = stored ? Color(1.000f, 0.620f, 0.078f) :
                                  Color(0.110f, 0.831f, 0.902f);
  const GlesColor light = stored ? Color(1.000f, 0.929f, 0.510f) :
                                   Color(0.655f, 1.000f, 0.992f);
  const GlesColor hot = Color(1.000f, 0.980f, 0.929f);

  // Dragon egg: an intentionally rounded, stepped sprite rather than a box.
  renderer->SetBlendMode(kGlesBlendOpaque);
  renderer->FillRect(GlesRect{rx + 4.0f * u, ry + 17.0f * u, 12.0f * u,
                              2.0f * u}, shadow);
  renderer->FillRect(GlesRect{rx + 5.0f * u, ry + 2.0f * u, 10.0f * u,
                              16.0f * u}, outline);
  renderer->FillRect(GlesRect{rx + 2.0f * u, ry + 7.0f * u, 16.0f * u,
                              8.0f * u}, outline);
  renderer->FillRect(GlesRect{rx + 6.0f * u, ry + u, 8.0f * u,
                              18.0f * u}, shell);
  renderer->FillRect(GlesRect{rx + 3.0f * u, ry + 8.0f * u, 14.0f * u,
                              6.0f * u}, shell);
  renderer->FillRect(GlesRect{rx + 5.0f * u, ry + 5.0f * u, 10.0f * u,
                              12.0f * u}, core);
  renderer->FillRect(GlesRect{rx + 4.0f * u, ry + 9.0f * u, 12.0f * u,
                              5.0f * u}, core);
  renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 14.0f * u, 8.0f * u,
                              2.0f * u}, shell);
  renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 4.0f * u, 4.0f * u,
                              3.0f * u}, light);
  renderer->FillRect(GlesRect{rx + 8.0f * u, ry + 3.0f * u, 2.0f * u,
                              u}, hot);
  renderer->FillRect(GlesRect{rx + 5.0f * u, ry + 10.0f * u, 2.0f * u,
                              2.0f * u}, light);
  renderer->FillRect(GlesRect{rx + 11.0f * u, ry + 8.0f * u, 2.0f * u,
                              u}, hot);
  renderer->FillRect(GlesRect{rx + 10.0f * u, ry + 12.0f * u, 3.0f * u,
                              u}, light);
}

void SokobanGame::DrawDragon(Gles2Renderer* renderer,
                             const HatchingDragon& dragon) {
  const float s = static_cast<float>(arcade::MaxInt(48, cell_ * 2));
  const float u = s / 20.0f;
  const float x = dragon.x - s * 0.5f;
  const float y = dragon.y - s * 0.5f;
  const GlesColor ink = Color(0.035f, 0.031f, 0.047f);
  const GlesColor wing_dark = Color(0.012f, 0.365f, 0.333f);
  const GlesColor wing = Color(0.024f, 0.827f, 0.647f);
  const GlesColor wing_light = Color(0.506f, 1.000f, 0.863f);
  const GlesColor body = Color(0.102f, 0.757f, 0.584f);
  const GlesColor scale = Color(0.231f, 0.945f, 0.714f);
  const GlesColor belly = Color(0.690f, 1.000f, 0.804f);
  const GlesColor eye = Color(1.000f, 0.773f, 0.125f);
  const bool wings_up = (static_cast<int>(hatch_timer_ms_ / 110.0) & 1) == 0;
  renderer->SetBlendMode(kGlesBlendOpaque);
  // One large stepped wing, mirrored around its shoulder between frames.
  if (wings_up) {
    renderer->FillRect(GlesRect{x + 5.0f * u, y + 5.0f * u, 6.0f * u,
                                7.0f * u}, ink);
    renderer->FillRect(GlesRect{x + 2.0f * u, y + 2.0f * u, 7.0f * u,
                                7.0f * u}, ink);
    renderer->FillRect(GlesRect{x, y, 5.0f * u, 7.0f * u}, ink);
    renderer->FillRect(GlesRect{x + 6.0f * u, y + 6.0f * u, 4.0f * u,
                                5.0f * u}, wing_dark);
    renderer->FillRect(GlesRect{x + 3.0f * u, y + 3.0f * u, 5.0f * u,
                                5.0f * u}, wing);
    renderer->FillRect(GlesRect{x + u, y + u, 3.0f * u, 5.0f * u}, wing_dark);
    renderer->FillRect(GlesRect{x + 4.0f * u, y + 4.0f * u, 3.0f * u,
                                u}, wing_light);
    renderer->FillRect(GlesRect{x + u, y + 2.0f * u, 2.0f * u,
                                u}, wing_light);
  } else {
    renderer->FillRect(GlesRect{x + 5.0f * u, y + 9.0f * u, 6.0f * u,
                                7.0f * u}, ink);
    renderer->FillRect(GlesRect{x + 2.0f * u, y + 12.0f * u, 7.0f * u,
                                7.0f * u}, ink);
    renderer->FillRect(GlesRect{x, y + 15.0f * u, 5.0f * u, 5.0f * u}, ink);
    renderer->FillRect(GlesRect{x + 6.0f * u, y + 10.0f * u, 4.0f * u,
                                5.0f * u}, wing_dark);
    renderer->FillRect(GlesRect{x + 3.0f * u, y + 13.0f * u, 5.0f * u,
                                5.0f * u}, wing);
    renderer->FillRect(GlesRect{x + u, y + 16.0f * u, 3.0f * u,
                                3.0f * u}, wing_dark);
    renderer->FillRect(GlesRect{x + 4.0f * u, y + 16.0f * u, 3.0f * u,
                                u}, wing_light);
    renderer->FillRect(GlesRect{x + u, y + 18.0f * u, 2.0f * u,
                                u}, wing_light);
  }
  renderer->FillRect(GlesRect{x + 5.0f * u, y + 10.0f * u, 10.0f * u,
                              5.0f * u}, ink);
  renderer->FillRect(GlesRect{x + 7.0f * u, y + 10.0f * u, 6.0f * u,
                              4.0f * u}, body);
  renderer->FillRect(GlesRect{x + 12.0f * u, y + 7.0f * u, 3.0f * u,
                              6.0f * u}, body);
  renderer->FillRect(GlesRect{x + 14.0f * u, y + 7.0f * u, 4.0f * u,
                              4.0f * u}, ink);
  renderer->FillRect(GlesRect{x + 15.0f * u, y + 6.0f * u, 2.0f * u,
                              5.0f * u}, body);
  renderer->FillRect(GlesRect{x + 16.0f * u, y + 5.0f * u, u,
                              2.0f * u}, scale);
  renderer->FillRect(GlesRect{x + 17.0f * u, y + 8.0f * u, u, u}, eye);
  renderer->FillRect(GlesRect{x + 13.0f * u, y + 5.0f * u, u,
                              2.0f * u}, belly);
  renderer->FillRect(GlesRect{x + 8.0f * u, y + 13.0f * u, 4.0f * u,
                              u}, belly);
  renderer->FillRect(GlesRect{x + 8.0f * u, y + 14.0f * u, 2.0f * u,
                              2.0f * u}, body);
  renderer->FillRect(GlesRect{x + 11.0f * u, y + 14.0f * u, 2.0f * u,
                              2.0f * u}, body);
  renderer->FillRect(GlesRect{x + 2.0f * u, y + 13.0f * u, 5.0f * u,
                              2.0f * u}, body);
  renderer->FillRect(GlesRect{x + u, y + 14.0f * u, 3.0f * u, u}, scale);
}

void SokobanGame::DrawRobot(Gles2Renderer* renderer, int x, int y) {
  {
    GlesRect tile = TileRect(x, y, 1.0f);
    const float u = tile.w / 20.0f;
    const float rx = tile.x;
    const float ry = tile.y;
    const GlesColor ink = Color(0.035f, 0.031f, 0.047f);
    const GlesColor steel_dark = Color(0.302f, 0.373f, 0.420f);
    const GlesColor steel = Color(0.733f, 0.812f, 0.843f);
    const GlesColor steel_light = Color(0.929f, 0.980f, 0.984f);
    const GlesColor visor = Color(0.071f, 0.247f, 0.278f);
    const GlesColor tabard = Color(0.835f, 0.137f, 0.192f);
    const GlesColor tabard_dark = Color(0.427f, 0.035f, 0.071f);

    // Directly follows the compact Slay knight silhouette: round helm,
    // single visor slit, scarlet tabard, then separate armoured limbs.
    renderer->SetBlendMode(kGlesBlendOpaque);
    renderer->FillRect(GlesRect{rx + 8.0f * u, ry + 1.0f * u, 4.0f * u,
                                2.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 3.0f * u, 8.0f * u,
                                7.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 5.0f * u, ry + 5.0f * u, 10.0f * u,
                                4.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 8.0f * u, ry + 2.0f * u, 4.0f * u,
                                8.0f * u}, steel);
    renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 5.0f * u, 8.0f * u,
                                3.0f * u}, steel);
    renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 3.0f * u, 3.0f * u,
                                2.0f * u}, steel_light);
    renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 6.0f * u, u,
                                2.0f * u}, steel_light);
    renderer->FillRect(GlesRect{rx + 12.0f * u, ry + 8.0f * u, 2.0f * u,
                                u}, steel_dark);
    renderer->FillRect(GlesRect{rx + 8.0f * u, ry + 6.0f * u, 4.0f * u,
                                3.0f * u}, visor);
    renderer->FillRect(GlesRect{rx + 9.0f * u, ry + 6.0f * u, u,
                                3.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 9.0f * u, ry + 7.0f * u, u,
                                u}, Color(0.259f, 0.761f, 0.753f));

    renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 10.0f * u, 8.0f * u,
                                6.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 11.0f * u, 6.0f * u,
                                5.0f * u}, tabard);
    renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 15.0f * u, 6.0f * u,
                                u}, tabard_dark);
    renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 14.0f * u, 6.0f * u,
                                u}, Color(0.965f, 0.706f, 0.184f));
    renderer->FillRect(GlesRect{rx + 3.0f * u, ry + 11.0f * u, 3.0f * u,
                                5.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 14.0f * u, ry + 11.0f * u, 3.0f * u,
                                5.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 4.0f * u, ry + 12.0f * u, 2.0f * u,
                                3.0f * u}, steel);
    renderer->FillRect(GlesRect{rx + 14.0f * u, ry + 12.0f * u, 2.0f * u,
                                3.0f * u}, steel);
    renderer->FillRect(GlesRect{rx + 4.0f * u, ry + 12.0f * u, 2.0f * u,
                                u}, steel_light);
    renderer->FillRect(GlesRect{rx + 14.0f * u, ry + 14.0f * u, 2.0f * u,
                                u}, steel_dark);
    renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 16.0f * u, 8.0f * u,
                                3.0f * u}, ink);
    renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 16.0f * u, 2.0f * u,
                                3.0f * u}, steel);
    renderer->FillRect(GlesRect{rx + 11.0f * u, ry + 16.0f * u, 2.0f * u,
                                3.0f * u}, steel);
    renderer->FillRect(GlesRect{rx + 7.0f * u, ry + 16.0f * u, 2.0f * u,
                                u}, steel_light);
    renderer->FillRect(GlesRect{rx + 6.0f * u, ry + 19.0f * u, 3.0f * u,
                                u}, steel_dark);
    renderer->FillRect(GlesRect{rx + 11.0f * u, ry + 19.0f * u, 3.0f * u,
                                u}, steel_dark);
  }
}

void SokobanGame::DrawHud(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::HudLayout layout = arcade::ComputeHudLayout(
      GlesRect{static_cast<float>(hud_x_), static_cast<float>(hud_y_),
               static_cast<float>(hud_w_), static_cast<float>(hud_h_)}, 3);
  arcade::DrawHudPanel(renderer, layout, style);
  arcade::HudFlow flow = arcade::BeginHudFlow(layout);
  arcade::DrawHudNumber(renderer, &flow, layout, "LEVEL", level_index_, 3,
                        p.speed, style);
  arcade::DrawHudNumber(renderer, &flow, layout, "MOVES", moves_, 3,
                        p.score, style);
  arcade::DrawHudNumber(renderer, &flow, layout, "PUSH", pushes_, 3,
                        p.score, style);
  arcade::DrawHudTextValue(renderer, &flow, layout, "SOUND",
                           sound_muted_ ? "OFF" : "ON",
                           sound_muted_ ? p.game_over_border : p.score, style);
}

void SokobanGame::DrawOverlay(Gles2Renderer* renderer) {
  const arcade::Palette& p = arcade::DefaultPalette();
  arcade::DrawCenteredOverlay(
      renderer, GlesRect{static_cast<float>(board_x_),
                         static_cast<float>(board_y_),
                         static_cast<float>(board_w_),
                         static_cast<float>(board_h_)},
      solved_notice_ ? "SOLVED" : "PAUSED",
      solved_notice_ ? p.board_border : p.pause_border,
      arcade::MaxInt(3, board_w_ / 180));
}

void SokobanGame::ActiveBounds(int* min_x, int* min_y, int* max_x,
                               int* max_y) const {
  int ax0 = kCols;
  int ay0 = kRows;
  int ax1 = -1;
  int ay1 = -1;
  for (int y = 0; y < kRows; ++y) {
    for (int x = 0; x < kCols; ++x) {
      if (!level_.walls[y][x]) continue;
      ax0 = arcade::MinInt(ax0, x);
      ay0 = arcade::MinInt(ay0, y);
      ax1 = arcade::MaxInt(ax1, x);
      ay1 = arcade::MaxInt(ay1, y);
    }
  }
  for (int i = 0; i < level_.box_count; ++i) {
    ax0 = arcade::MinInt(ax0, level_.boxes[i].x);
    ay0 = arcade::MinInt(ay0, level_.boxes[i].y);
    ax1 = arcade::MaxInt(ax1, level_.boxes[i].x);
    ay1 = arcade::MaxInt(ay1, level_.boxes[i].y);
    ax0 = arcade::MinInt(ax0, level_.goals[i].x);
    ay0 = arcade::MinInt(ay0, level_.goals[i].y);
    ax1 = arcade::MaxInt(ax1, level_.goals[i].x);
    ay1 = arcade::MaxInt(ay1, level_.goals[i].y);
  }
  ax0 = arcade::MinInt(ax0, level_.player.x);
  ay0 = arcade::MinInt(ay0, level_.player.y);
  ax1 = arcade::MaxInt(ax1, level_.player.x);
  ay1 = arcade::MaxInt(ay1, level_.player.y);
  if (ax1 < ax0 || ay1 < ay0) {
    ax0 = 0;
    ay0 = 0;
    ax1 = kCols - 1;
    ay1 = kRows - 1;
  }
  if (min_x) *min_x = ax0;
  if (min_y) *min_y = ay0;
  if (max_x) *max_x = ax1;
  if (max_y) *max_y = ay1;
}

GlesRect SokobanGame::TileRect(int x, int y, float inset) const {
  int min_x = 0;
  int min_y = 0;
  int max_x = kCols - 1;
  int max_y = kRows - 1;
  ActiveBounds(&min_x, &min_y, &max_x, &max_y);
  int active_w = arcade::MaxInt(1, max_x - min_x + 1);
  int active_h = arcade::MaxInt(1, max_y - min_y + 1);
  float content_inset = static_cast<float>(arcade::MaxInt(6, cell_ / 5));
  float content_w = static_cast<float>(board_w_) - content_inset * 2.0f;
  float content_h = static_cast<float>(board_h_) - content_inset * 2.0f;
  float tile = content_w / static_cast<float>(active_w);
  float tile_h = content_h / static_cast<float>(active_h);
  if (tile_h < tile) tile = tile_h;
  float max_tile = static_cast<float>(cell_) * 1.35f;
  if (tile > max_tile) tile = max_tile;
  float view_w = tile * static_cast<float>(active_w);
  float view_h = tile * static_cast<float>(active_h);
  float ox = static_cast<float>(board_x_) + content_inset +
             (content_w - view_w) * 0.5f;
  float oy = static_cast<float>(board_y_) + content_inset +
             (content_h - view_h) * 0.5f;
  return GlesRect{ox + static_cast<float>(x - min_x) * tile + inset,
                  oy + static_cast<float>(y - min_y) * tile + inset,
                  tile - inset * 2.0f, tile - inset * 2.0f};
}
