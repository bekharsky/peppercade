#include "ascii_sprite.h"
#include "color_util.h"

AsciiSpritePalette MakeAsciiSpritePalette(GlesColor outline, GlesColor shadow,
                                          GlesColor base, GlesColor light,
                                          GlesColor highlight,
                                          GlesColor accent, GlesColor glow) {
  AsciiSpritePalette palette;
  palette.inks[kAsciiOutline] = outline;
  palette.inks[kAsciiDeepShadow] = arcade::MixColors(outline, shadow, 0.35f);
  palette.inks[kAsciiShadow] = shadow;
  palette.inks[kAsciiDark] = arcade::MixColors(shadow, base, 0.52f);
  palette.inks[kAsciiBase] = base;
  palette.inks[kAsciiLight] = light;
  palette.inks[kAsciiHighlight] = highlight;
  palette.inks[kAsciiSpecular] = arcade::MixColors(highlight, light, 0.30f);
  palette.inks[kAsciiAccent] = accent;
  palette.inks[kAsciiAccentLight] = arcade::MixColors(accent, highlight, 0.52f);
  palette.inks[kAsciiSoftGlow] = glow;
  palette.inks[kAsciiSoftGlow].a *= 0.30f;
  palette.inks[kAsciiGlow] = glow;
  palette.inks[kAsciiHotGlow] = arcade::MixColors(glow, highlight, 0.65f);
  return palette;
}

void DrawAsciiSprite(Gles2Renderer* renderer, const AsciiSprite& sprite,
                     GlesRect bounds, const AsciiSpritePalette& palette) {
  if (!renderer || !sprite.runs || sprite.run_count <= 0 ||
      sprite.row_count <= 0 ||
      sprite.column_count <= 0) return;
  const int padding_x = sprite.padding_x < 0 ? 0 : sprite.padding_x;
  const int padding_y = sprite.padding_y < 0 ? 0 : sprite.padding_y;
  const float logical_w =
      static_cast<float>(sprite.column_count + padding_x * 2);
  const float logical_h =
      static_cast<float>(sprite.row_count + padding_y * 2);
  const float cell_w = bounds.w / logical_w;
  const float cell_h = bounds.h / logical_h;
  const float clip_left =
      static_cast<float>(static_cast<int>(bounds.x + 0.5f));
  const float clip_top =
      static_cast<float>(static_cast<int>(bounds.y + 0.5f));
  const float clip_right =
      static_cast<float>(static_cast<int>(bounds.x + bounds.w + 0.5f));
  const float clip_bottom =
      static_cast<float>(static_cast<int>(bounds.y + bounds.h + 0.5f));
  renderer->SetBlendMode(sprite.blend_mode == kAsciiBlendAlpha
                             ? kGlesBlendAlpha
                             : kGlesBlendOpaque);
  for (int i = 0; i < sprite.run_count; ++i) {
    const AsciiSpriteRun& run = sprite.runs[i];
    if (run.ink < 0 || run.ink >= kAsciiInkCount || run.length <= 0) continue;
    // Round and overlap by one pixel to avoid fractional-scale seams. A run is
    // emitted as one rectangle, so equal neighboring cells do not overdraw.
    const float left = bounds.x + (run.x + padding_x) * cell_w;
    const float top = bounds.y + (run.y + padding_y) * cell_h;
    const float right = bounds.x +
                        (run.x + run.length + padding_x) * cell_w;
    const float bottom = bounds.y + (run.y + padding_y + 1) * cell_h;
    const float snapped_left =
        static_cast<float>(static_cast<int>(left + 0.5f));
    const float snapped_top =
        static_cast<float>(static_cast<int>(top + 0.5f));
    const float snapped_right =
        static_cast<float>(static_cast<int>(right + 0.5f));
    const float snapped_bottom =
        static_cast<float>(static_cast<int>(bottom + 0.5f));
    const float draw_left = snapped_left < clip_left ? clip_left : snapped_left;
    const float draw_top = snapped_top < clip_top ? clip_top : snapped_top;
    const float expanded_right = snapped_right + 1.0f;
    const float expanded_bottom = snapped_bottom + 1.0f;
    const float draw_right =
        expanded_right > clip_right ? clip_right : expanded_right;
    const float draw_bottom =
        expanded_bottom > clip_bottom ? clip_bottom : expanded_bottom;
    if (draw_right <= draw_left || draw_bottom <= draw_top) continue;
    renderer->FillRect(GlesRect{draw_left, draw_top,
                                draw_right - draw_left,
                                draw_bottom - draw_top},
                       palette.inks[run.ink]);
  }
}

void DrawAsciiSpriteAt(Gles2Renderer* renderer, const AsciiSprite& sprite,
                       float anchor_x, float anchor_y, float cell_width,
                       float cell_height, const AsciiSpritePalette& palette) {
  const float width =
      (sprite.column_count + sprite.padding_x * 2) * cell_width;
  const float height =
      (sprite.row_count + sprite.padding_y * 2) * cell_height;
  DrawAsciiSprite(renderer, sprite,
                  GlesRect{anchor_x - width * sprite.anchor_x,
                           anchor_y - height * sprite.anchor_y, width, height},
                  palette);
}
