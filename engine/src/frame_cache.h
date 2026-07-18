#ifndef CLASSIC_GAME_KIT_FRAME_CACHE_H_
#define CLASSIC_GAME_KIT_FRAME_CACHE_H_

#include "pixel_canvas.h"

#include <stdint.h>
#include <vector>

class StaticFrameCache {
 public:
  StaticFrameCache();

  void Resize(int width, int height);
  void Invalidate();
  bool NeedsRedraw(bool bgra) const;
  PixelCanvas* BeginRedraw(bool bgra);
  void EndRedraw();
  void CopyTo(uint32_t* pixels) const;

 private:
  std::vector<uint32_t> pixels_;
  PixelCanvas canvas_;
  int width_;
  int height_;
  bool dirty_;
  bool bgra_;
};

#endif  // CLASSIC_GAME_KIT_FRAME_CACHE_H_
