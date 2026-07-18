#ifndef TIZEN_TETRIS_PATTERNS_H_
#define TIZEN_TETRIS_PATTERNS_H_

#include <stdint.h>

struct TetrisColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct TetrisPattern {
  TetrisColor background;
  TetrisColor primary;
  TetrisColor secondary;
  TetrisColor accent;
  int motif;
  int step_extra;
  int phase;
};

int TetrisPatternVariantCount();
TetrisPattern TetrisPatternForLines(int lines);

#endif  // TIZEN_TETRIS_PATTERNS_H_
