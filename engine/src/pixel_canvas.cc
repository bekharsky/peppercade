#include "pixel_canvas.h"
#include "math_util.h"

PixelCanvas::PixelCanvas()
    : pixels_(0),
      width_(0),
      height_(0),
      stride_(0),
      origin_x_(0),
      origin_y_(0),
      bgra_(false) {}

void PixelCanvas::SetTarget(uint32_t* pixels, int width, int height,
                            bool bgra) {
  SetTargetRegion(pixels, width, height, width, 0, 0, bgra);
}

void PixelCanvas::SetTargetRegion(uint32_t* pixels, int width, int height,
                                  int stride, int origin_x, int origin_y,
                                  bool bgra) {
  pixels_ = pixels;
  width_ = width;
  height_ = height;
  stride_ = stride;
  origin_x_ = origin_x;
  origin_y_ = origin_y;
  bgra_ = bgra;
}

uint32_t PixelCanvas::Pack(GameColor c) const {
  if (bgra_) return 0xff000000u | (c.b << 16) | (c.g << 8) | c.r;
  return 0xff000000u | (c.r << 16) | (c.g << 8) | c.b;
}

void PixelCanvas::PutPixel(int x, int y, GameColor c) {
  x -= origin_x_;
  y -= origin_y_;
  if (!pixels_ || x < 0 || y < 0 || x >= width_ || y >= height_) return;
  pixels_[y * stride_ + x] = Pack(c);
}

void PixelCanvas::FillRect(int x, int y, int w, int h, GameColor c) {
  if (!pixels_ || w <= 0 || h <= 0) return;
  int x0 = arcade::ClampInt(x - origin_x_, 0, width_);
  int y0 = arcade::ClampInt(y - origin_y_, 0, height_);
  int x1 = arcade::ClampInt(x + w - origin_x_, 0, width_);
  int y1 = arcade::ClampInt(y + h - origin_y_, 0, height_);
  uint32_t packed = Pack(c);
  for (int yy = y0; yy < y1; ++yy) {
    uint32_t* row = pixels_ + yy * stride_;
    for (int xx = x0; xx < x1; ++xx) row[xx] = packed;
  }
}

void PixelCanvas::OutlineRect(int x, int y, int w, int h, GameColor c, int t) {
  if (t <= 0) return;
  FillRect(x, y, w, t, c);
  FillRect(x, y + h - t, w, t, c);
  FillRect(x, y, t, h, c);
  FillRect(x + w - t, y, t, h, c);
}
