#ifndef CLASSIC_GAME_KIT_MATH_UTIL_H_
#define CLASSIC_GAME_KIT_MATH_UTIL_H_

// Tiny scalar helpers shared by game code.  Keep these header-only so every
// platform target gets identical behavior without another build-script entry.
namespace arcade {

inline int MinInt(int a, int b) { return a < b ? a : b; }
inline int MaxInt(int a, int b) { return a > b ? a : b; }
inline int AbsInt(int value) { return value < 0 ? -value : value; }
inline int ClampInt(int value, int lo, int hi) {
  return value < lo ? lo : (value > hi ? hi : value);
}

inline float MinFloat(float a, float b) { return a < b ? a : b; }
inline float MaxFloat(float a, float b) { return a > b ? a : b; }
inline float ClampFloat(float value, float lo, float hi) {
  return value < lo ? lo : (value > hi ? hi : value);
}

inline int CycleInt(int value, int lo, int hi) {
  if (hi < lo) return lo;
  ++value;
  return value > hi ? lo : value;
}

}  // namespace arcade

#endif  // CLASSIC_GAME_KIT_MATH_UTIL_H_
