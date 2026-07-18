#include "arcade_hud.h"
#include "math_util.h"

namespace arcade {

HudStyle DefaultHudStyle() {
  const Palette& p = DefaultPalette();
  HudStyle style = {p.hud_panel, p.hud_border, p.label, p.score, p.speed,
                    Rgb(38, 34, 53)};
  return style;
}

int ScreenMargin(int width, int height) {
  int short_side = width < height ? width : height;
  return ClampInt(short_side / 24, 8, 32);
}

int SideHudGap(int width) { return ClampInt(width / 80, 6, 24); }

int SideHudWidth(int width) { return ClampInt(width / 6, 108, 320); }

GridSideLayout ComputeGridSideLayout(int width, int height, int columns,
                                     int rows, int min_cell,
                                     bool framed_playfield) {
  const int margin = ScreenMargin(width, height);
  const int gap = SideHudGap(width);
  const int hud_w = SideHudWidth(width);
  const int border = framed_playfield ? PanelBorderSize(hud_w) : 0;
  const int available_w =
      width - margin * 2 - gap - hud_w - border * 2;
  const int available_h = height - margin * 2 - border * 2;
  const int cell = MaxInt(min_cell,
                          MinInt(available_w / columns, available_h / rows));
  const int play_w = cell * columns;
  const int play_h = cell * rows;
  const int framed_w = play_w + border * 2;
  const int framed_h = play_h + border * 2;
  int play_x = (width - framed_w - gap - hud_w) / 2 + border;
  if (play_x - border < margin) play_x = margin + border;
  const int play_y = (height - framed_h) / 2 + border;
  return GridSideLayout{cell,
                        play_x,
                        play_y,
                        play_w,
                        play_h,
                        border,
                        play_x + play_w + border + gap,
                        play_y - border,
                        hud_w,
                        framed_h};
}

HudLayout ComputeHudLayout(GlesRect panel, int max_digits) {
  if (max_digits < 1) max_digits = 1;
  int panel_w = static_cast<int>(panel.w);
  int panel_h = static_cast<int>(panel.h);
  int padding = ClampInt(panel_w / 10, 6, 22);
  int label_scale = panel_h >= 760 ? 3 : (panel_h >= 480 ? 2 : 1);
  int digit_size = panel_h >= 760 ? 26 : (panel_h >= 480 ? 20 : 14);
  int scale_by_width = (panel_w - padding * 2) / 29;
  label_scale = ClampInt(label_scale, 1, ClampInt(scale_by_width, 1, 3));
  int digit_by_width =
      (panel_w - padding * 2 - (max_digits - 1) * 8) / max_digits;
  digit_size = ClampInt(digit_size, 8, ClampInt(digit_by_width, 8, 26));
  int item_gap = panel_h >= 760 ? 24 : (panel_h >= 480 ? 18 : 10);

  HudLayout layout = {panel,
                      static_cast<int>(panel.x) + padding,
                      static_cast<int>(panel.y) + padding,
                      panel_w - padding * 2,
                      padding,
                      item_gap,
                      label_scale,
                      digit_size,
                      static_cast<float>(PanelBorderSize(panel_w))};
  return layout;
}

HudFlow BeginHudFlow(const HudLayout& layout) {
  HudFlow flow = {layout.content_y, layout.item_gap};
  return flow;
}

int ReserveHudBlock(HudFlow* flow, int height) {
  if (!flow) return 0;
  int y = flow->y;
  flow->y += height + flow->gap;
  return y;
}

void DrawHudPanel(Gles2Renderer* renderer, const HudLayout& layout,
                  const HudStyle& style) {
  DrawHudPanel(renderer, layout.panel, layout.border_size, style);
}

void DrawHudNumber(Gles2Renderer* renderer, HudFlow* flow,
                   const HudLayout& layout, const char* label, int value,
                   int digits, GlesColor value_color,
                   const HudStyle& style) {
  int thickness = layout.digit_size / 5;
  if (thickness < 4) thickness = 4;
  int height = layout.label_scale * 18 + layout.digit_size * 2 + thickness;
  int y = ReserveHudBlock(flow, height);
  DrawHudNumber(renderer, label, value, digits, layout.content_x, y,
                layout.label_scale, layout.digit_size, value_color, style);
}

void DrawHudTextValue(Gles2Renderer* renderer, HudFlow* flow,
                      const HudLayout& layout, const char* label,
                      const char* value, GlesColor value_color,
                      const HudStyle& style) {
  int height = layout.label_scale * 23;
  int y = ReserveHudBlock(flow, height);
  DrawHudTextValue(renderer, label, value, layout.content_x, y,
                   layout.label_scale, value_color, style);
}

void DrawHudProgress(Gles2Renderer* renderer, HudFlow* flow,
                     const HudLayout& layout, const char* label, int value,
                     int max_value, int bar_h, const HudStyle& style) {
  int height = layout.label_scale * 18 + bar_h;
  int y = ReserveHudBlock(flow, height);
  DrawHudProgress(renderer, label, value, max_value, layout.content_x, y,
                  layout.content_w, layout.label_scale, bar_h, style);
}

void DrawHudPanel(Gles2Renderer* renderer, GlesRect rect, float border_size,
                  const HudStyle& style) {
  DrawPanel(renderer, rect, style.panel, style.border, border_size);
}

void DrawHudNumber(Gles2Renderer* renderer, const char* label, int value,
                   int digits, int x, int y, int label_scale, int digit_size,
                   GlesColor value_color, const HudStyle& style) {
  DrawPixelText(renderer, label, x, y, label_scale, style.label);
  DrawSevenSegmentNumber(renderer, x, y + label_scale * 18, digit_size, value,
                         digits, value_color, style.off);
}

void DrawHudTextValue(Gles2Renderer* renderer, const char* label,
                      const char* value, int x, int y, int label_scale,
                      GlesColor value_color, const HudStyle& style) {
  DrawPixelText(renderer, label, x, y, label_scale, style.label);
  DrawPixelText(renderer, value, x, y + label_scale * 16, label_scale,
                value_color);
}

void DrawHudProgress(Gles2Renderer* renderer, const char* label, int value,
                     int max_value, int x, int y, int w, int label_scale,
                     int bar_h, const HudStyle& style) {
  DrawPixelText(renderer, label, x, y, label_scale, style.label);
  int bar_y = y + label_scale * 18;
  renderer->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(bar_y),
                              static_cast<float>(w), static_cast<float>(bar_h)},
                     style.off);
  int clamped = ClampInt(value, 0, max_value);
  int fill_w = max_value > 0 ? w * clamped / max_value : 0;
  renderer->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(bar_y),
                              static_cast<float>(fill_w),
                              static_cast<float>(bar_h)},
                     style.value);
}

void DrawCenteredOverlay(Gles2Renderer* renderer, GlesRect bounds,
                         const char* text, GlesColor border, int scale) {
  const Palette& p = DefaultPalette();
  // One responsive treatment for every game. The old callers supplied
  // unrelated scale values, which made Bubble Shooter tiny and Tetris huge.
  const float card_w = bounds.w * 0.72f;
  const float card_h = bounds.h * 0.24f;
  bounds.x += (bounds.w - card_w) * 0.5f;
  bounds.y += (bounds.h - card_h) * 0.5f;
  bounds.w = card_w;
  bounds.h = card_h;
  const int max_phrase_width = PixelTextWidth("GAME OVER", 1);
  const int width_scale =
      (static_cast<int>(bounds.w) - 64) / max_phrase_width;
  const int height_scale = (static_cast<int>(bounds.h) - 36) / 7;
  scale = ClampInt(width_scale < height_scale ? width_scale : height_scale,
                   4, 8);
  DrawPanel(renderer, bounds, p.overlay_panel, border, 7.0f);
  int text_w = PixelTextWidth(text, scale);
  DrawPixelText(renderer, text,
                static_cast<int>(bounds.x + (bounds.w - text_w) * 0.5f),
                static_cast<int>(bounds.y + (bounds.h - 7 * scale) * 0.5f),
                scale, p.overlay_text);
}

}  // namespace arcade
