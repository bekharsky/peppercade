#ifndef CLASSIC_GAME_KIT_COLOR_UTIL_H_
#define CLASSIC_GAME_KIT_COLOR_UTIL_H_

#include "gles2_renderer.h"
#include "math_util.h"

namespace arcade {

inline GlesColor MixColors(GlesColor a, GlesColor b, float amount) {
  const float t = ClampFloat(amount, 0.0f, 1.0f);
  const float keep = 1.0f - t;
  return GlesColor{a.r * keep + b.r * t, a.g * keep + b.g * t,
                   a.b * keep + b.b * t, a.a * keep + b.a * t};
}

inline GlesColor ScaleRgb(GlesColor color, float factor) {
  color.r = ClampFloat(color.r * factor, 0.0f, 1.0f);
  color.g = ClampFloat(color.g * factor, 0.0f, 1.0f);
  color.b = ClampFloat(color.b * factor, 0.0f, 1.0f);
  return color;
}

inline GlesColor Darken(GlesColor color, float factor = 0.62f) {
  return ScaleRgb(color, factor);
}

inline GlesColor Lighten(GlesColor color, float amount = 0.38f) {
  const float t = ClampFloat(amount, 0.0f, 1.0f);
  color.r += (1.0f - color.r) * t;
  color.g += (1.0f - color.g) * t;
  color.b += (1.0f - color.b) * t;
  return color;
}

inline float ColorLuma(GlesColor color) {
  return color.r * 0.2126f + color.g * 0.7152f + color.b * 0.0722f;
}

}  // namespace arcade

#endif  // CLASSIC_GAME_KIT_COLOR_UTIL_H_
