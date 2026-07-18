#include "../src/collision.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

namespace {

bool Near(float actual, float expected) {
  return fabsf(actual - expected) < 0.001f;
}

void TestLeadingEdgeHitsBeforeCenterEntersBrick() {
  const arcade::CollisionRect brick = {222.0f + 5.0f * 46.923077f,
                                        48.0f + 7.0f * 31.0f,
                                        42.923077f, 27.0f};
  arcade::CircleRectHit hit = {};
  assert(arcade::CircleIntersectsRect(466.8f, 299.3f, 14.0f, brick, &hit));
  assert(hit.normal_y > 0.99f);
  assert(hit.penetration > 0.0f);
}

void TestSideAndCornerContacts() {
  const arcade::CollisionRect brick = {100.0f, 100.0f, 40.0f, 20.0f};
  arcade::CircleRectHit hit = {};
  assert(arcade::CircleIntersectsRect(92.0f, 110.0f, 10.0f, brick, &hit));
  assert(Near(hit.normal_x, -1.0f));
  assert(Near(hit.normal_y, 0.0f));
  assert(Near(hit.penetration, 2.0f));

  assert(arcade::CircleIntersectsRect(94.0f, 94.0f, 9.0f, brick, &hit));
  assert(hit.normal_x < -0.70f);
  assert(hit.normal_y < -0.70f);
}

void TestMissAndCenterInsideFallback() {
  const arcade::CollisionRect brick = {100.0f, 100.0f, 40.0f, 20.0f};
  arcade::CircleRectHit hit = {};
  assert(!arcade::CircleIntersectsRect(80.0f, 110.0f, 10.0f, brick, &hit));

  assert(arcade::CircleIntersectsRect(105.0f, 110.0f, 4.0f, brick, &hit));
  assert(Near(hit.normal_x, -1.0f));
  assert(Near(hit.normal_y, 0.0f));
  assert(Near(hit.penetration, 9.0f));
}

}  // namespace

int main() {
  TestLeadingEdgeHitsBeforeCenterEntersBrick();
  TestSideAndCornerContacts();
  TestMissAndCenterInsideFallback();
  puts("collision tests passed");
  return 0;
}
