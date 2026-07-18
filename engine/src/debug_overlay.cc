#include "debug_overlay.h"

#include "arcade_style.h"

#include <stdio.h>

namespace debug_overlay {

bool Enabled() { return PEPPERCADE_DEBUG_OVERLAY != 0; }

void DrawFps(Gles2Renderer* renderer, double fps) {
  if (!Enabled() || !renderer) return;
  int value = static_cast<int>(fps + 0.5);
  if (value < 0) value = 0;
  if (value > 99) value = 99;
  renderer->SetBlendMode(kGlesBlendAlpha);
  renderer->FillRect(GlesRect{8.0f, 8.0f, 128.0f, 42.0f},
                     GlesColor{0.02f, 0.025f, 0.055f, 0.72f});
  arcade::DrawPixelText(renderer, "FPS", 14, 15, 2,
                        GlesColor{0.72f, 0.98f, 0.92f, 1.0f});
  arcade::DrawSevenSegmentNumber(renderer, 70, 13, 10, value, 2,
                                 GlesColor{1.0f, 0.85f, 0.52f, 1.0f},
                                 GlesColor{0.15f, 0.13f, 0.20f, 1.0f});
}

}  // namespace debug_overlay
