#include "stackfall_patterns.h"

namespace {

const TetrisPattern kPatterns[] = {
    {{11, 16, 25}, {78, 105, 121}, {171, 139, 117}, {95, 139, 123}, 0, 0, 0},
    {{13, 15, 28}, {95, 93, 132}, {181, 137, 157}, {119, 151, 142}, 1, 18, 1},
    {{12, 18, 23}, {86, 126, 104}, {190, 158, 101}, {112, 119, 158}, 2, 6, 2},
    {{17, 14, 28}, {118, 94, 128}, {145, 158, 122}, {190, 130, 104}, 3, 28, 3},
    {{10, 19, 27}, {75, 124, 138}, {143, 109, 148}, {176, 158, 118}, 4, 14, 4},
    {{18, 16, 24}, {119, 105, 88}, {95, 141, 132}, {148, 129, 170}, 5, 34, 5},
    {{12, 15, 30}, {90, 105, 140}, {174, 136, 106}, {124, 158, 118}, 6, 10, 6},
    {{18, 13, 25}, {111, 91, 122}, {101, 137, 147}, {188, 164, 112}, 7, 24, 7},
};

}  // namespace

int TetrisPatternVariantCount() {
  return static_cast<int>(sizeof(kPatterns) / sizeof(kPatterns[0]));
}

TetrisPattern TetrisPatternForLines(int lines) {
  int count = TetrisPatternVariantCount();
  int bucket = lines / 5;
  return kPatterns[bucket % count];
}
