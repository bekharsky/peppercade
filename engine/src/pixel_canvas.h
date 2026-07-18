#ifndef CLASSIC_GAME_KIT_PIXEL_CANVAS_H_
#define CLASSIC_GAME_KIT_PIXEL_CANVAS_H_

#include <stdint.h>

struct GameColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class PixelCanvas {
 public:
  PixelCanvas();

  void SetTarget(uint32_t* pixels, int width, int height, bool bgra);
  void SetTargetRegion(uint32_t* pixels, int width, int height, int stride,
                       int origin_x, int origin_y, bool bgra);
  uint32_t Pack(GameColor c) const;
  void PutPixel(int x, int y, GameColor c);
  void FillRect(int x, int y, int w, int h, GameColor c);
  void OutlineRect(int x, int y, int w, int h, GameColor c, int t);

 private:
  uint32_t* pixels_;
  int width_;
  int height_;
  int stride_;
  int origin_x_;
  int origin_y_;
  bool bgra_;
};

#endif  // CLASSIC_GAME_KIT_PIXEL_CANVAS_H_
