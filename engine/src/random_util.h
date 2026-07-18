#ifndef CLASSIC_GAME_KIT_RANDOM_UTIL_H_
#define CLASSIC_GAME_KIT_RANDOM_UTIL_H_

#include <stdint.h>

namespace arcade {

// Deterministic LCG used by gameplay and effects.  Game instances own the
// state so resets and platform backends never depend on global randomness.
inline uint32_t NextRandom(uint32_t* state) {
  *state = *state * 1664525u + 1013904223u;
  return *state;
}

}  // namespace arcade

#endif  // CLASSIC_GAME_KIT_RANDOM_UTIL_H_
