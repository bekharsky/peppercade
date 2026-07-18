#ifndef CLASSIC_GAME_KIT_PIXEL_EFFECTS_H_
#define CLASSIC_GAME_KIT_PIXEL_EFFECTS_H_

#include "gles2_renderer.h"

#include <stdint.h>

namespace pixel_effects {

struct Particle {
  float x;
  float y;
  float vx;
  float vy;
  float life;
  float max_life;
  float size;
  int color;
  bool active;
};

struct Burst {
  float x;
  float y;
  float life;
  float max_life;
  int color;
  bool active;
};

void SpawnParticles(Particle* particles, int max_particles, float x, float y,
                    int color, int count, uint32_t* rng);
void SpawnBurst(Burst* bursts, int max_bursts, float x, float y, int color);
void UpdateParticles(Particle* particles, int max_particles, double dt_ms);
void UpdateBursts(Burst* bursts, int max_bursts, double dt_ms);
void DrawParticles(Gles2Renderer* renderer, const Particle* particles,
                   int max_particles, const GlesColor* colors,
                   int color_count);
void DrawBursts(Gles2Renderer* renderer, const Burst* bursts, int max_bursts,
                const GlesColor* colors, int color_count, float base_size,
                float border_size);

}  // namespace pixel_effects

#endif  // CLASSIC_GAME_KIT_PIXEL_EFFECTS_H_
