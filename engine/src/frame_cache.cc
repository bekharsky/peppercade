#include "frame_cache.h"

#include <string.h>

StaticFrameCache::StaticFrameCache()
    : width_(0), height_(0), dirty_(true), bgra_(false) {}

void StaticFrameCache::Resize(int width, int height) {
  if (width == width_ && height == height_) return;
  width_ = width;
  height_ = height;
  pixels_.assign(width_ * height_, 0);
  dirty_ = true;
}

void StaticFrameCache::Invalidate() { dirty_ = true; }

bool StaticFrameCache::NeedsRedraw(bool bgra) const {
  return dirty_ || bgra_ != bgra;
}

PixelCanvas* StaticFrameCache::BeginRedraw(bool bgra) {
  bgra_ = bgra;
  if (pixels_.empty()) return 0;
  canvas_.SetTarget(&pixels_[0], width_, height_, bgra);
  return &canvas_;
}

void StaticFrameCache::EndRedraw() { dirty_ = false; }

void StaticFrameCache::CopyTo(uint32_t* pixels) const {
  if (pixels_.empty() || !pixels) return;
  memcpy(pixels, &pixels_[0],
         static_cast<size_t>(width_) * static_cast<size_t>(height_) *
             sizeof(uint32_t));
}
