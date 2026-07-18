#ifndef CLASSIC_GAME_KIT_GAME_CATALOG_H_
#define CLASSIC_GAME_KIT_GAME_CATALOG_H_

#include "game.h"

namespace peppercade {

enum GameId {
  kGameTetris = 0,
  kGameArkanoid,
  kGame2048,
  kGameSnake,
  kGameTron,
  kGameSokoban,
  kGameBubbleShooter,
  kGameCount
};

inline GameId NextGame(GameId game, int delta) {
  int value = static_cast<int>(game) + delta;
  while (value < 0) value += kGameCount;
  while (value >= kGameCount) value -= kGameCount;
  return static_cast<GameId>(value);
}

inline const char* GameLabel(GameId game) {
  static const char* const kLabels[kGameCount] = {
      "STACKFALL", "BRICKBOUND", "2048", "NEON SERPENT", "LIGHTLINE",
      "DRAGOBAN", "BUBBLE ORBIT"};
  return game >= 0 && game < kGameCount ? kLabels[game] : "";
}

class GameCatalog {
 public:
  GameCatalog() {
    for (int i = 0; i < kGameCount; ++i) games_[i] = 0;
  }

  void Set(GameId id, Game* game) { games_[id] = game; }
  Game* Get(GameId id) { return games_[id]; }
  const Game* Get(GameId id) const { return games_[id]; }

  void ResizeAll(int width, int height) {
    for (int i = 0; i < kGameCount; ++i)
      if (games_[i]) games_[i]->Resize(width, height);
  }

 private:
  Game* games_[kGameCount];
};

}  // namespace peppercade

#endif  // CLASSIC_GAME_KIT_GAME_CATALOG_H_
