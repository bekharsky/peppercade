#ifndef CLASSIC_GAME_KIT_COLLISION_H_
#define CLASSIC_GAME_KIT_COLLISION_H_

#include <math.h>

namespace arcade {

struct CollisionRect {
  float x;
  float y;
  float w;
  float h;
};

struct CircleRectHit {
  float normal_x;
  float normal_y;
  float penetration;
};

inline float CollisionClamp(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

// Returns the outward-facing collision normal and overlap depth. This handles
// both edge/corner contacts and the fallback case where the circle center has
// already entered the rectangle during the current simulation step.
inline bool CircleIntersectsRect(float circle_x, float circle_y, float radius,
                                 const CollisionRect& rect,
                                 CircleRectHit* hit) {
  if (!hit || radius < 0.0f || rect.w <= 0.0f || rect.h <= 0.0f) return false;

  const float closest_x =
      CollisionClamp(circle_x, rect.x, rect.x + rect.w);
  const float closest_y =
      CollisionClamp(circle_y, rect.y, rect.y + rect.h);
  const float dx = circle_x - closest_x;
  const float dy = circle_y - closest_y;
  const float distance_sq = dx * dx + dy * dy;
  if (distance_sq > radius * radius) return false;

  if (distance_sq > 0.000001f) {
    const float distance = sqrtf(distance_sq);
    hit->normal_x = dx / distance;
    hit->normal_y = dy / distance;
    hit->penetration = radius - distance;
    return true;
  }

  const float to_left = circle_x - rect.x;
  const float to_right = rect.x + rect.w - circle_x;
  const float to_top = circle_y - rect.y;
  const float to_bottom = rect.y + rect.h - circle_y;
  float nearest = to_left;
  hit->normal_x = -1.0f;
  hit->normal_y = 0.0f;
  if (to_right < nearest) {
    nearest = to_right;
    hit->normal_x = 1.0f;
    hit->normal_y = 0.0f;
  }
  if (to_top < nearest) {
    nearest = to_top;
    hit->normal_x = 0.0f;
    hit->normal_y = -1.0f;
  }
  if (to_bottom < nearest) {
    nearest = to_bottom;
    hit->normal_x = 0.0f;
    hit->normal_y = 1.0f;
  }
  hit->penetration = radius + nearest;
  return true;
}

}  // namespace arcade

#endif  // CLASSIC_GAME_KIT_COLLISION_H_
