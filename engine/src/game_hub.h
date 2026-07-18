#ifndef CLASSIC_GAME_KIT_GAME_HUB_H_
#define CLASSIC_GAME_KIT_GAME_HUB_H_

#include "game_catalog.h"

namespace peppercade {

// Shared selector rendering used by SDL/WebAssembly and NaCl shells.
void DrawGameHub(Gles2Renderer* renderer, int width, int height, int variant,
                 GameId selected);

}  // namespace peppercade

#endif  // CLASSIC_GAME_KIT_GAME_HUB_H_
