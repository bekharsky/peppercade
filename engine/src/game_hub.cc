#include "game_hub.h"

#include "arcade_hud.h"
#include "arcade_style.h"
#include "math_util.h"

namespace peppercade {
namespace {

void DrawHubRow(Gles2Renderer* renderer, const char* label, GameId game,
                GameId selected_game, int x, int y, int scale, int width,
                int height) {
  const arcade::Palette& palette = arcade::DefaultPalette();
  const bool selected = selected_game == game;
  if (selected) {
    const GlesColor color = palette.pieces[static_cast<int>(game) % 7];
    renderer->FillRect(GlesRect{static_cast<float>(x - 18),
                                static_cast<float>(y),
                                static_cast<float>(width + 44),
                                static_cast<float>(height)},
                       color);
  }
  arcade::DrawPixelText(renderer, label, x, y + (height - 7 * scale) / 2,
                        scale, selected ? palette.hud_panel : palette.label);
}

}  // namespace

void DrawGameHub(Gles2Renderer* renderer, int width, int height, int variant,
                 GameId selected) {
  if (!renderer) return;
  const arcade::Palette& palette = arcade::DefaultPalette();
  renderer->BeginFrame(palette.screen_bg);
  arcade::DrawBackgroundPattern(
      renderer, GlesRect{0, 0, static_cast<float>(width),
                         static_cast<float>(height)},
      variant + static_cast<int>(selected) * 7, width / 13);

  const int margin = arcade::MaxInt(
      12, arcade::MinInt(40, arcade::MinInt(width, height) / 18));
  const int panel_w = arcade::MaxInt(
      360, arcade::MinInt(width - margin * 2, width * 52 / 100));
  const int title_scale = arcade::MaxInt(
      2, arcade::MinInt(panel_w >= 360 ? 4 : 3, (panel_w - 40) / 59));
  const int title_h = title_scale * 7;
  const int title_gap = arcade::MaxInt(8, margin / 3);
  const int panel_h = arcade::MaxInt(
      260, arcade::MinInt(height - margin * 2 - title_h - title_gap,
                          height * 78 / 100));
  const int x = (width - panel_w) / 2;
  const int total_h = title_h + title_gap + panel_h;
  const int title_y = (height - total_h) / 2;
  const int y = title_y + title_h + title_gap;

  const arcade::HudStyle style = arcade::DefaultHudStyle();
  arcade::DrawHudPanel(renderer,
                       GlesRect{static_cast<float>(x), static_cast<float>(y),
                                static_cast<float>(panel_w),
                                static_cast<float>(panel_h)},
                       static_cast<float>(arcade::PanelBorderSize(panel_w)),
                       style);
  int row_scale = (panel_w >= 600 || panel_h >= 700) ? 5 : 4;
  row_scale = arcade::MaxInt(
      1, arcade::MinInt(row_scale, (panel_w - 64) / 47));
  const int top_pad = arcade::MaxInt(16, arcade::MinInt(28, panel_h / 14));
  const int list_y = y + top_pad;
  const int row_step = arcade::MaxInt(
      row_scale * 9, (panel_h - (list_y - y) - top_pad) / kGameCount);
  const int item_h = arcade::MinInt(row_step - 4, row_scale * 10 + 24);

  arcade::DrawPixelText(renderer, "PEPPERCADE", x + 5, y - title_h - 10,
                        title_scale, palette.overlay_text);
  for (int i = 0; i < kGameCount; ++i) {
    DrawHubRow(renderer, GameLabel(static_cast<GameId>(i)),
               static_cast<GameId>(i), selected, x + 42,
               list_y + i * row_step + (row_step - item_h) / 2, row_scale,
               panel_w - 84, item_h);
  }
}

}  // namespace peppercade
