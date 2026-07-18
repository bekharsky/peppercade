#ifndef CLASSIC_GAME_KIT_ASCII_SPRITE_H_
#define CLASSIC_GAME_KIT_ASCII_SPRITE_H_

#include "gles2_renderer.h"

enum AsciiSpriteInk {
  kAsciiOutline = 0,       // .
  kAsciiDeepShadow,       // _
  kAsciiShadow,           // -
  kAsciiDark,             // :
  kAsciiBase,             // =
  kAsciiLight,            // ^
  kAsciiHighlight,        // *
  kAsciiSpecular,         // !
  kAsciiAccent,           // @
  kAsciiAccentLight,      // %
  kAsciiSoftGlow,         // ~
  kAsciiGlow,             // +
  kAsciiHotGlow,          // #
  kAsciiInkCount
};

enum AsciiSpriteBlendMode {
  kAsciiBlendOpaque = 0,
  kAsciiBlendAlpha
};

// Generated from a same-color horizontal run in a .pascii source. Keeping
// runs instead of individual cells preserves the editable grid while avoiding
// one GPU quad per painted character.
struct AsciiSpriteRun {
  int x;
  int y;
  int length;
  AsciiSpriteInk ink;
};

struct AsciiSprite {
  const AsciiSpriteRun* runs;
  int run_count;
  int row_count;
  int column_count;
  int padding_x;
  int padding_y;
  float anchor_x;
  float anchor_y;
  AsciiSpriteBlendMode blend_mode;
};

struct AsciiSpritePalette {
  GlesColor inks[kAsciiInkCount];
};

// Builds the expanded ink ramp from the original seven material colors.
// Callers can override individual palette.inks entries afterwards.
AsciiSpritePalette MakeAsciiSpritePalette(GlesColor outline, GlesColor shadow,
                                          GlesColor base, GlesColor light,
                                          GlesColor highlight,
                                          GlesColor accent, GlesColor glow);

// Draws generated runs into bounds. Every resource may use its own grid
// resolution, padding, anchor metadata, and blend mode.
void DrawAsciiSprite(Gles2Renderer* renderer, const AsciiSprite& sprite,
                     GlesRect bounds, const AsciiSpritePalette& palette);

// Draws at a logical point using the resource anchor and native grid aspect.
// This is useful when differently sized sprite variants share one world point.
void DrawAsciiSpriteAt(Gles2Renderer* renderer, const AsciiSprite& sprite,
                       float anchor_x, float anchor_y, float cell_width,
                       float cell_height, const AsciiSpritePalette& palette);

#endif  // CLASSIC_GAME_KIT_ASCII_SPRITE_H_
