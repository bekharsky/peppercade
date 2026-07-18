#ifndef CLASSIC_GAME_KIT_ARCADE_HUD_H_
#define CLASSIC_GAME_KIT_ARCADE_HUD_H_

#include "arcade_style.h"

namespace arcade {

struct HudStyle {
  GlesColor panel;
  GlesColor border;
  GlesColor label;
  GlesColor value;
  GlesColor value_alt;
  GlesColor off;
};

struct HudLayout {
  GlesRect panel;
  int content_x;
  int content_y;
  int content_w;
  int padding;
  int item_gap;
  int label_scale;
  int digit_size;
  float border_size;
};

struct HudFlow {
  int y;
  int gap;
};

// Canonical geometry for a fixed-cell grid beside the standard HUD.  Snake,
// Lightline, and Dragoban share this shape; game renderers only choose whether
// the playfield border sits outside the logical grid.
struct GridSideLayout {
  int cell;
  int play_x;
  int play_y;
  int play_w;
  int play_h;
  int border;
  int hud_x;
  int hud_y;
  int hud_w;
  int hud_h;
};

HudStyle DefaultHudStyle();
int ScreenMargin(int width, int height);
int SideHudGap(int width);
int SideHudWidth(int width);
GridSideLayout ComputeGridSideLayout(int width, int height, int columns,
                                     int rows, int min_cell,
                                     bool framed_playfield);
HudLayout ComputeHudLayout(GlesRect panel, int max_digits);
HudFlow BeginHudFlow(const HudLayout& layout);
int ReserveHudBlock(HudFlow* flow, int height);
void DrawHudPanel(Gles2Renderer* renderer, const HudLayout& layout,
                  const HudStyle& style);
void DrawHudNumber(Gles2Renderer* renderer, HudFlow* flow,
                   const HudLayout& layout, const char* label, int value,
                   int digits, GlesColor value_color,
                   const HudStyle& style);
void DrawHudTextValue(Gles2Renderer* renderer, HudFlow* flow,
                      const HudLayout& layout, const char* label,
                      const char* value, GlesColor value_color,
                      const HudStyle& style);
void DrawHudProgress(Gles2Renderer* renderer, HudFlow* flow,
                     const HudLayout& layout, const char* label, int value,
                     int max_value, int bar_h, const HudStyle& style);

void DrawHudPanel(Gles2Renderer* renderer, GlesRect rect, float border_size,
                  const HudStyle& style);
void DrawHudNumber(Gles2Renderer* renderer, const char* label, int value,
                   int digits, int x, int y, int label_scale, int digit_size,
                   GlesColor value_color, const HudStyle& style);
void DrawHudTextValue(Gles2Renderer* renderer, const char* label,
                      const char* value, int x, int y, int label_scale,
                      GlesColor value_color, const HudStyle& style);
void DrawHudProgress(Gles2Renderer* renderer, const char* label, int value,
                     int max_value, int x, int y, int w, int label_scale,
                     int bar_h, const HudStyle& style);
void DrawCenteredOverlay(Gles2Renderer* renderer, GlesRect bounds,
                         const char* text, GlesColor border, int scale);

}  // namespace arcade

#endif  // CLASSIC_GAME_KIT_ARCADE_HUD_H_
