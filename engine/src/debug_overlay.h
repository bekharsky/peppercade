#ifndef CLASSIC_GAME_KIT_DEBUG_OVERLAY_H_
#define CLASSIC_GAME_KIT_DEBUG_OVERLAY_H_

#include "gles2_renderer.h"

#ifndef PEPPERCADE_DEBUG_OVERLAY
#define PEPPERCADE_DEBUG_OVERLAY 1
#endif

namespace debug_overlay {

bool Enabled();
void DrawFps(Gles2Renderer* renderer, double fps);

}  // namespace debug_overlay

#endif  // CLASSIC_GAME_KIT_DEBUG_OVERLAY_H_
