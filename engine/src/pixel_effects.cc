#include "pixel_effects.h"

#include "random_util.h"

#include "arcade_style.h"

#include <math.h>

namespace pixel_effects {

void SpawnParticles(Particle* particles, int max_particles, float x, float y,
                    int color, int count, uint32_t* rng) {
  if (!particles || !rng) return;
  for (int n = 0; n < count; ++n) {
    for (int i = 0; i < max_particles; ++i) {
      if (particles[i].active) continue;
      particles[i].active = true;
      particles[i].x = x;
      particles[i].y = y;
      float angle = static_cast<float>((arcade::NextRandom(rng) % 628) / 100.0f);
      float speed = static_cast<float>(90 + arcade::NextRandom(rng) % 250);
      particles[i].vx = cosf(angle) * speed;
      particles[i].vy = sinf(angle) * speed;
      particles[i].life = particles[i].max_life =
          static_cast<float>(220 + arcade::NextRandom(rng) % 360);
      particles[i].size = static_cast<float>(4 + arcade::NextRandom(rng) % 9);
      particles[i].color = color;
      break;
    }
  }
}

void SpawnBurst(Burst* bursts, int max_bursts, float x, float y, int color) {
  if (!bursts) return;
  for (int i = 0; i < max_bursts; ++i) {
    if (bursts[i].active) continue;
    bursts[i].active = true;
    bursts[i].x = x;
    bursts[i].y = y;
    bursts[i].life = bursts[i].max_life = 180.0f;
    bursts[i].color = color;
    return;
  }
}

void UpdateParticles(Particle* particles, int max_particles, double dt_ms) {
  if (!particles) return;
  float dt = static_cast<float>(dt_ms / 1000.0);
  for (int i = 0; i < max_particles; ++i) {
    Particle& p = particles[i];
    if (!p.active) continue;
    p.life -= static_cast<float>(dt_ms);
    if (p.life <= 0.0f) {
      p.active = false;
      continue;
    }
    p.vy += 180.0f * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
  }
}

void UpdateBursts(Burst* bursts, int max_bursts, double dt_ms) {
  if (!bursts) return;
  for (int i = 0; i < max_bursts; ++i) {
    Burst& b = bursts[i];
    if (!b.active) continue;
    b.life -= static_cast<float>(dt_ms);
    if (b.life <= 0.0f) b.active = false;
  }
}

void DrawParticles(Gles2Renderer* renderer, const Particle* particles,
                   int max_particles, const GlesColor* colors,
                   int color_count) {
  if (!renderer || !particles || !colors || color_count <= 0) return;
  for (int i = 0; i < max_particles; ++i) {
    const Particle& p = particles[i];
    if (!p.active) continue;
    GlesColor c = colors[p.color % color_count];
    c.a = p.life / p.max_life;
    renderer->FillRect(GlesRect{p.x, p.y, p.size, p.size}, c);
  }
}

void DrawBursts(Gles2Renderer* renderer, const Burst* bursts, int max_bursts,
                const GlesColor* colors, int color_count, float base_size,
                float border_size) {
  if (!renderer || !bursts || !colors || color_count <= 0) return;
  (void)border_size;
  for (int i = 0; i < max_bursts; ++i) {
    const Burst& b = bursts[i];
    if (!b.active) continue;
    float t = b.life / b.max_life;
    GlesColor c = colors[b.color % color_count];
    c.a = t;
    // Bursts are deliberately pixel-only. Keep the radial pattern stable and
    // let the pixels travel outward; never draw a scalable panel or outline.
    static const float kCos[] = {1.0f, 0.707f, 0.0f, -0.707f,
                                 -1.0f, -0.707f, 0.0f, 0.707f};
    static const float kSin[] = {0.0f, 0.707f, 1.0f, 0.707f,
                                 0.0f, -0.707f, -1.0f, -0.707f};
    float distance = (1.0f - t) * base_size * 0.45f;
    float pixel = base_size > 30.0f ? 5.0f : 3.0f;
    for (int p = 0; p < 8; ++p) {
      renderer->FillRect(GlesRect{b.x + kCos[p] * distance - pixel * 0.5f,
                                  b.y + kSin[p] * distance - pixel * 0.5f,
                                  pixel, pixel},
                         c);
    }
  }
}

}  // namespace pixel_effects
