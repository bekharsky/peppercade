#ifndef CLASSIC_GAME_KIT_ARCADE_STYLE_H_
#define CLASSIC_GAME_KIT_ARCADE_STYLE_H_

#include "gles2_renderer.h"

#include <stdint.h>

namespace arcade {

GlesColor Rgb(uint8_t r, uint8_t g, uint8_t b);
GlesColor Rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

struct Palette {
  GlesColor screen_bg;
  GlesColor playfield_bg;
  GlesColor grid;
  GlesColor board_border;
  GlesColor hud_panel;
  GlesColor hud_border;
  GlesColor label;
  GlesColor score;
  GlesColor speed;
  GlesColor overlay_panel;
  GlesColor overlay_text;
  GlesColor game_over_border;
  GlesColor pause_border;
  GlesColor block_shadow;
  GlesColor block_highlight;
  GlesColor pieces[7];
};

const Palette& DefaultPalette();
int PanelBorderSize(int reference_width);

void DrawPixelText(Gles2Renderer* renderer, const char* text, int x, int y,
                   int scale, GlesColor color);
int PixelTextWidth(const char* text, int scale);
void DrawBackgroundPattern(Gles2Renderer* renderer, GlesRect bounds,
                           int variant, int min_step);

void DrawSevenSegmentDigit(Gles2Renderer* renderer, int x, int y, int size,
                           int value, GlesColor on, GlesColor off);
void DrawSevenSegmentNumber(Gles2Renderer* renderer, int x, int y, int size,
                            int value, int digits, GlesColor on,
                            GlesColor off);

void DrawGlossBlock(Gles2Renderer* renderer, GlesRect rect, GlesColor base);
void DrawPanel(Gles2Renderer* renderer, GlesRect rect, GlesColor fill,
               GlesColor border, float border_size);
void DrawBorder(Gles2Renderer* renderer, GlesRect rect, GlesColor border,
                float border_size);

}  // namespace arcade

#endif  // CLASSIC_GAME_KIT_ARCADE_STYLE_H_
