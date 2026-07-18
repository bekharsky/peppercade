#include "arcade_style.h"
#include "math_util.h"

#include <stddef.h>

namespace arcade {
namespace {

const uint8_t* Glyph(char ch) {
  static const uint8_t space[7] = {0, 0, 0, 0, 0, 0, 0};
  static const uint8_t bang[7] = {4, 4, 4, 4, 4, 0, 4};
  static const uint8_t quote[7] = {10, 10, 10, 0, 0, 0, 0};
  static const uint8_t hash[7] = {10, 10, 31, 10, 31, 10, 10};
  static const uint8_t percent[7] = {24, 25, 2, 4, 8, 19, 3};
  static const uint8_t amp[7] = {12, 18, 20, 8, 21, 18, 13};
  static const uint8_t apostrophe[7] = {4, 4, 8, 0, 0, 0, 0};
  static const uint8_t left_paren[7] = {2, 4, 8, 8, 8, 4, 2};
  static const uint8_t right_paren[7] = {8, 4, 2, 2, 2, 4, 8};
  static const uint8_t star[7] = {0, 4, 21, 14, 21, 4, 0};
  static const uint8_t plus[7] = {0, 4, 4, 31, 4, 4, 0};
  static const uint8_t comma[7] = {0, 0, 0, 0, 4, 4, 8};
  static const uint8_t minus[7] = {0, 0, 0, 31, 0, 0, 0};
  static const uint8_t dot[7] = {0, 0, 0, 0, 0, 12, 12};
  static const uint8_t slash[7] = {1, 2, 2, 4, 8, 8, 16};
  static const uint8_t zero[7] = {14, 17, 19, 21, 25, 17, 14};
  static const uint8_t one[7] = {4, 12, 4, 4, 4, 4, 14};
  static const uint8_t two[7] = {14, 17, 1, 2, 4, 8, 31};
  static const uint8_t three[7] = {30, 1, 1, 14, 1, 1, 30};
  static const uint8_t four[7] = {2, 6, 10, 18, 31, 2, 2};
  static const uint8_t five[7] = {31, 16, 16, 30, 1, 1, 30};
  static const uint8_t six[7] = {14, 16, 16, 30, 17, 17, 14};
  static const uint8_t seven[7] = {31, 1, 2, 4, 8, 8, 8};
  static const uint8_t eight[7] = {14, 17, 17, 14, 17, 17, 14};
  static const uint8_t nine[7] = {14, 17, 17, 15, 1, 1, 14};
  static const uint8_t colon[7] = {0, 12, 12, 0, 12, 12, 0};
  static const uint8_t semicolon[7] = {0, 12, 12, 0, 4, 4, 8};
  static const uint8_t less[7] = {2, 4, 8, 16, 8, 4, 2};
  static const uint8_t equal[7] = {0, 0, 31, 0, 31, 0, 0};
  static const uint8_t greater[7] = {8, 4, 2, 1, 2, 4, 8};
  static const uint8_t question[7] = {14, 17, 1, 2, 4, 0, 4};
  static const uint8_t at[7] = {14, 17, 23, 21, 23, 16, 14};
  static const uint8_t a[7] = {14, 17, 17, 31, 17, 17, 17};
  static const uint8_t b[7] = {30, 17, 17, 30, 17, 17, 30};
  static const uint8_t c[7] = {14, 17, 16, 16, 16, 17, 14};
  static const uint8_t d[7] = {30, 17, 17, 17, 17, 17, 30};
  static const uint8_t e[7] = {31, 16, 16, 30, 16, 16, 31};
  static const uint8_t f[7] = {31, 16, 16, 30, 16, 16, 16};
  static const uint8_t g[7] = {14, 17, 16, 23, 17, 17, 15};
  static const uint8_t h[7] = {17, 17, 17, 31, 17, 17, 17};
  static const uint8_t i[7] = {14, 4, 4, 4, 4, 4, 14};
  static const uint8_t j[7] = {7, 2, 2, 2, 2, 18, 12};
  static const uint8_t k[7] = {17, 18, 20, 24, 20, 18, 17};
  static const uint8_t l[7] = {16, 16, 16, 16, 16, 16, 31};
  static const uint8_t m[7] = {17, 27, 21, 21, 17, 17, 17};
  static const uint8_t n[7] = {17, 25, 21, 19, 17, 17, 17};
  static const uint8_t o[7] = {14, 17, 17, 17, 17, 17, 14};
  static const uint8_t p[7] = {30, 17, 17, 30, 16, 16, 16};
  static const uint8_t q[7] = {14, 17, 17, 17, 21, 18, 13};
  static const uint8_t r[7] = {30, 17, 17, 30, 20, 18, 17};
  static const uint8_t s[7] = {15, 16, 16, 14, 1, 1, 30};
  static const uint8_t t[7] = {31, 4, 4, 4, 4, 4, 4};
  static const uint8_t u[7] = {17, 17, 17, 17, 17, 17, 14};
  static const uint8_t v[7] = {17, 17, 17, 17, 17, 10, 4};
  static const uint8_t w[7] = {17, 17, 17, 21, 21, 21, 10};
  static const uint8_t x[7] = {17, 17, 10, 4, 10, 17, 17};
  static const uint8_t y[7] = {17, 17, 10, 4, 4, 4, 4};
  static const uint8_t z[7] = {31, 1, 2, 4, 8, 16, 31};
  static const uint8_t left_bracket[7] = {14, 8, 8, 8, 8, 8, 14};
  static const uint8_t backslash[7] = {16, 8, 8, 4, 2, 2, 1};
  static const uint8_t right_bracket[7] = {14, 2, 2, 2, 2, 2, 14};
  static const uint8_t caret[7] = {4, 10, 17, 0, 0, 0, 0};
  static const uint8_t underscore[7] = {0, 0, 0, 0, 0, 0, 31};
  switch (ch) {
    case ' ': return space;
    case '!': return bang;
    case '"': return quote;
    case '#': return hash;
    case '%': return percent;
    case '&': return amp;
    case '\'': return apostrophe;
    case '(': return left_paren;
    case ')': return right_paren;
    case '*': return star;
    case '+': return plus;
    case ',': return comma;
    case '-': return minus;
    case '.': return dot;
    case '/': return slash;
    case '0': return zero;
    case '1': return one;
    case '2': return two;
    case '3': return three;
    case '4': return four;
    case '5': return five;
    case '6': return six;
    case '7': return seven;
    case '8': return eight;
    case '9': return nine;
    case ':': return colon;
    case ';': return semicolon;
    case '<': return less;
    case '=': return equal;
    case '>': return greater;
    case '?': return question;
    case '@': return at;
    case 'A':
    case 'a': return a;
    case 'B':
    case 'b': return b;
    case 'C':
    case 'c': return c;
    case 'D':
    case 'd': return d;
    case 'E':
    case 'e': return e;
    case 'F':
    case 'f': return f;
    case 'G':
    case 'g': return g;
    case 'H':
    case 'h': return h;
    case 'I':
    case 'i': return i;
    case 'J':
    case 'j': return j;
    case 'K':
    case 'k': return k;
    case 'L':
    case 'l': return l;
    case 'M':
    case 'm': return m;
    case 'N':
    case 'n': return n;
    case 'O':
    case 'o': return o;
    case 'P':
    case 'p': return p;
    case 'Q':
    case 'q': return q;
    case 'R':
    case 'r': return r;
    case 'S':
    case 's': return s;
    case 'T':
    case 't': return t;
    case 'U':
    case 'u': return u;
    case 'V':
    case 'v': return v;
    case 'W':
    case 'w': return w;
    case 'X':
    case 'x': return x;
    case 'Y':
    case 'y': return y;
    case 'Z':
    case 'z': return z;
    case '[': return left_bracket;
    case '\\': return backslash;
    case ']': return right_bracket;
    case '^': return caret;
    case '_': return underscore;
    default: return space;
  }
}

}  // namespace

GlesColor Rgb(uint8_t r, uint8_t g, uint8_t b) {
  return Rgba(r, g, b, 255);
}

GlesColor Rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return GlesColor{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

const Palette& DefaultPalette() {
  static const Palette palette = {
      Rgb(5, 7, 18),
      Rgb(5, 9, 20),
      Rgb(17, 44, 61),
      Rgb(255, 209, 102),
      Rgb(13, 13, 34),
      Rgb(165, 132, 154),
      Rgb(172, 200, 192),
      Rgb(125, 225, 204),
      Rgb(255, 216, 132),
      Rgb(7, 12, 25),
      Rgb(235, 222, 184),
      Rgb(198, 92, 112),
      Rgb(207, 180, 118),
      Rgb(20, 22, 38),
      Rgb(255, 255, 255),
      {Rgb(36, 221, 255), Rgb(87, 118, 255), Rgb(255, 154, 33),
       Rgb(255, 216, 84), Rgb(55, 224, 155), Rgb(190, 104, 255),
       Rgb(255, 82, 114)}};
  return palette;
}

int PanelBorderSize(int reference_width) {
  int size = reference_width / 55;
  if (size < 2) size = 2;
  if (size > 5) size = 5;
  return size;
}

void DrawPixelText(Gles2Renderer* renderer, const char* text, int x, int y,
                   int scale, GlesColor color) {
  if (!renderer || !text || scale <= 0) return;
  int cursor = x;
  for (const char* p = text; *p; ++p) {
    const uint8_t* rows = Glyph(*p);
    for (int yy = 0; yy < 7; ++yy) {
      for (int xx = 0; xx < 5; ++xx) {
        if (rows[yy] & (1 << (4 - xx))) {
          renderer->FillRect(GlesRect{static_cast<float>(cursor + xx * scale),
                                      static_cast<float>(y + yy * scale),
                                      static_cast<float>(scale),
                                      static_cast<float>(scale)},
                             color);
        }
      }
    }
    cursor += 6 * scale;
  }
}

int PixelTextWidth(const char* text, int scale) {
  if (!text || scale <= 0) return 0;
  int len = 0;
  while (text[len]) ++len;
  if (len == 0) return 0;
  return len * 6 * scale - scale;
}

void DrawBackgroundPattern(Gles2Renderer* renderer, GlesRect bounds,
                           int variant, int min_step) {
  if (!renderer || bounds.w <= 0.0f || bounds.h <= 0.0f) return;
  const Palette& p = DefaultPalette();
  const GlesColor backgrounds[] = {
      Rgb(7, 10, 24), Rgb(9, 13, 28), Rgb(7, 18, 26), Rgb(16, 11, 29),
      Rgb(14, 18, 24), Rgb(8, 17, 18), Rgb(18, 12, 24)};
  const int bg_count =
      static_cast<int>(sizeof(backgrounds) / sizeof(backgrounds[0]));
  if (variant < 0) variant = -variant;
  renderer->FillRect(bounds, backgrounds[variant % bg_count]);

  int step = min_step;
  if (step < 48) step = 48;
  step += (variant % 5) * 10;
  const int start_x = static_cast<int>(bounds.x) - step;
  const int start_y = static_cast<int>(bounds.y) - step;
  const int end_x = static_cast<int>(bounds.x + bounds.w) + step;
  const int end_y = static_cast<int>(bounds.y + bounds.h) + step;

  renderer->SetBlendMode(kGlesBlendAlpha);
  for (int y = start_y; y < end_y; y += step) {
    for (int x = start_x; x < end_x; x += step) {
      int gx = (x - static_cast<int>(bounds.x)) / step;
      int gy = (y - static_cast<int>(bounds.y)) / step;
      int selector = (gx * 3 + gy * 5 + variant) & 3;
      GlesColor c = selector == 0 ? p.grid :
                    (selector == 1 ? p.hud_border :
                     (selector == 2 ? p.label : p.pieces[variant % 7]));
      c.a = 0.26f;
      int motif = variant % 6;
      if (motif == 0) {
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 5),
                                    static_cast<float>(y + step / 5),
                                    static_cast<float>(step * 3 / 5),
                                    static_cast<float>(step / 8)}, c);
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 5),
                                    static_cast<float>(y + step / 5),
                                    static_cast<float>(step / 8),
                                    static_cast<float>(step * 3 / 5)}, c);
      } else if (motif == 1) {
        DrawPanel(renderer,
                  GlesRect{static_cast<float>(x + step / 5),
                           static_cast<float>(y + step / 5),
                           static_cast<float>(step * 3 / 5),
                           static_cast<float>(step * 3 / 5)},
                  backgrounds[variant % bg_count], c,
                  static_cast<float>(step / 14 > 4 ? step / 14 : 4));
      } else if (motif == 2) {
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 3),
                                    static_cast<float>(y + step / 3),
                                    static_cast<float>(step / 3),
                                    static_cast<float>(step / 3)}, c);
      } else if (motif == 3) {
        int thick = step / 9 > 8 ? step / 9 : 8;
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 4),
                                    static_cast<float>(y + step / 2),
                                    static_cast<float>(step / 2),
                                    static_cast<float>(thick)}, c);
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 2),
                                    static_cast<float>(y + step / 4),
                                    static_cast<float>(thick),
                                    static_cast<float>(step / 2)}, c);
      } else if (motif == 4) {
        int thick = step / 7 > 10 ? step / 7 : 10;
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 8),
                                    static_cast<float>(y + step / 6),
                                    static_cast<float>(step * 3 / 4),
                                    static_cast<float>(thick)}, c);
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 3),
                                    static_cast<float>(y + step / 6),
                                    static_cast<float>(thick),
                                    static_cast<float>(step / 2)}, c);
      } else {
        renderer->FillRect(GlesRect{static_cast<float>(x + step / 10),
                                    static_cast<float>(y + step / 10),
                                    static_cast<float>(step / 5),
                                    static_cast<float>(step / 5)}, c);
        renderer->FillRect(GlesRect{static_cast<float>(x + step * 6 / 10),
                                    static_cast<float>(y + step * 6 / 10),
                                    static_cast<float>(step / 4),
                                    static_cast<float>(step / 4)}, c);
      }
    }
  }
  renderer->SetBlendMode(kGlesBlendOpaque);
}

void DrawSevenSegmentDigit(Gles2Renderer* renderer, int x, int y, int size,
                           int value, GlesColor on, GlesColor off) {
  if (!renderer || size <= 0) return;
  static const uint8_t map[10] = {
      0x77, 0x12, 0x5d, 0x5b, 0x3a, 0x6b, 0x6f, 0x52, 0x7f, 0x7b};
  const uint8_t m = map[value % 10];
  const int t = size / 5 > 4 ? size / 5 : 4;
  const int w = size;
  const int h = size * 2;
  renderer->FillRect(GlesRect{static_cast<float>(x + t), static_cast<float>(y),
                              static_cast<float>(w - 2 * t),
                              static_cast<float>(t)},
                     (m & 0x40) ? on : off);
  renderer->FillRect(GlesRect{static_cast<float>(x), static_cast<float>(y + t),
                              static_cast<float>(t),
                              static_cast<float>(h / 2 - t)},
                     (m & 0x20) ? on : off);
  renderer->FillRect(GlesRect{static_cast<float>(x + w - t),
                              static_cast<float>(y + t),
                              static_cast<float>(t),
                              static_cast<float>(h / 2 - t)},
                     (m & 0x10) ? on : off);
  renderer->FillRect(GlesRect{static_cast<float>(x + t),
                              static_cast<float>(y + h / 2),
                              static_cast<float>(w - 2 * t),
                              static_cast<float>(t)},
                     (m & 0x08) ? on : off);
  renderer->FillRect(GlesRect{static_cast<float>(x),
                              static_cast<float>(y + h / 2 + t),
                              static_cast<float>(t),
                              static_cast<float>(h / 2 - t)},
                     (m & 0x04) ? on : off);
  renderer->FillRect(GlesRect{static_cast<float>(x + w - t),
                              static_cast<float>(y + h / 2 + t),
                              static_cast<float>(t),
                              static_cast<float>(h / 2 - t)},
                     (m & 0x02) ? on : off);
  renderer->FillRect(GlesRect{static_cast<float>(x + t),
                              static_cast<float>(y + h),
                              static_cast<float>(w - 2 * t),
                              static_cast<float>(t)},
                     (m & 0x01) ? on : off);
}

void DrawSevenSegmentNumber(Gles2Renderer* renderer, int x, int y, int size,
                            int value, int digits, GlesColor on,
                            GlesColor off) {
  if (!renderer || digits <= 0) return;
  int divisor = 1;
  for (int i = 1; i < digits; ++i) divisor *= 10;
  for (int i = 0; i < digits; ++i) {
    DrawSevenSegmentDigit(renderer, x + i * (size + 8), y, size,
                          (value / divisor) % 10, on, off);
    divisor /= 10;
  }
}

void DrawGlossBlock(Gles2Renderer* renderer, GlesRect rect, GlesColor base) {
  if (!renderer) return;
  const Palette& p = DefaultPalette();
  renderer->FillRect(rect, base);
  const float pad = MaxFloat(3.0f, rect.w / 9.0f);
  renderer->FillRect(GlesRect{rect.x + pad, rect.y + pad,
                              rect.w - pad * 2.0f,
                              MaxFloat(2.0f, rect.h / 9.0f)},
                     p.block_highlight);
  renderer->FillRect(GlesRect{rect.x + pad,
                              rect.y + rect.h - MaxFloat(5.0f, rect.h / 7.0f),
                              rect.w - pad * 2.0f,
                              MaxFloat(2.0f, rect.h / 10.0f)},
                     p.block_shadow);
}

void DrawPanel(Gles2Renderer* renderer, GlesRect rect, GlesColor fill,
               GlesColor border, float border_size) {
  if (!renderer || border_size <= 0.0f) return;
  renderer->FillRect(rect, fill);
  DrawBorder(renderer, rect, border, border_size);
}

void DrawBorder(Gles2Renderer* renderer, GlesRect rect, GlesColor border,
                float border_size) {
  if (!renderer || border_size <= 0.0f) return;
  renderer->FillRect(GlesRect{rect.x, rect.y, rect.w, border_size}, border);
  renderer->FillRect(GlesRect{rect.x, rect.y + rect.h - border_size, rect.w,
                              border_size},
                     border);
  renderer->FillRect(GlesRect{rect.x, rect.y, border_size, rect.h}, border);
  renderer->FillRect(GlesRect{rect.x + rect.w - border_size, rect.y,
                              border_size, rect.h},
                     border);
}

}  // namespace arcade
