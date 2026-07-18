# Engine Survey

Generated: 2026-07-09

Goal: choose a practical base for high-performance 2D arcade games on old
Samsung Tizen TV NaCl now, with a future WebAssembly path.

## Requirements

- Fast 2D sprite rendering for 8-bit/16-bit-style arcade games.
- Predictable frame pacing on old TV hardware.
- Keyboard-like TV remote/gamepad input.
- Portable C or C++ game core.
- Native macOS development build.
- Samsung TV NaCl build today.
- Samsung TV WebAssembly build later.

## Candidates

### SDL2 / SDL3

SDL is a low-level cross-platform multimedia layer for graphics, input, audio,
and game controllers. It is widely used by games and emulators.

Pros:

- Best fit for input, window/event loop, audio, and portability.
- Emscripten explicitly supports SDL; SDL apps usually need little input/output
  change to run in the browser.
- Emscripten has a maintained SDL2 port through `--use-port=sdl2` /
  `-sUSE_SDL=2`.
- Old NaCl SDL backend work existed historically, so the model is plausible
  even if we should not depend on an unmaintained backend.

Cons:

- SDL is not a full game engine. We still need sprites, animation, scenes,
  collision helpers, asset pipeline, and UI conventions.
- Existing NaCl backend code is historical, not something to trust blindly on
  Samsung Pepper 47.

Verdict:

Use SDL as the reference platform abstraction, especially for macOS and
WebAssembly. For NaCl, implement a small compatible backend over PPAPI instead
of trying to resurrect a full old SDL port immediately.

### raylib

raylib is a small C game-programming library with textures, shapes, input,
audio, and web builds via Emscripten.

Pros:

- Much nicer game-facing API than raw SDL.
- Good match for fast prototypes, sprite sheets, simple arcade games, and
  teaching-style code.
- Official docs include texture, input, audio, and web examples.
- Web builds are a known use case.

Cons:

- Web backend currently depends on Emscripten/GLFW/WebGL style integration, not
  our old Samsung NaCl/PPAPI target.
- It is not a sprite/entity engine by itself; higher-level runtime pieces would
  still be ours.
- Pulling all of raylib into Pepper 47 NaCl may cost more time than copying the
  parts of the API style we actually need.

Verdict:

Good API inspiration and WebAssembly experiment candidate. Risky as the core
dependency for old NaCl.

### Cocos2d-x

Cocos2d-x is a mature C++ 2D game engine lineage.

Pros:

- Real scene graph, sprites, actions, animation concepts.
- Historically used for mobile 2D games.

Cons:

- Emscripten support appears historical/deprecated in older discussions.
- Too large for the old Samsung NaCl target.
- High integration risk before we have measured the TV rendering path.

Verdict:

Do not use as the base. Keep as conceptual reference for scene/actions APIs.

### Godot / Other Full Engines

Pros:

- Complete editor and runtime.
- Mature 2D features.

Cons:

- Wrong shape for Samsung NaCl.
- Heavy runtime, complex export story, and too much platform surface for this
  TV experiment.

Verdict:

Not a candidate for this repository.

## Recommended Architecture

Build Peppercade as a thin game runtime, not a full imported engine:

```text
game code
  -> peppercade runtime: sprites, animations, scenes, collisions, timers
    -> backend interface: frame, texture/surface, input, audio, metrics
      -> macOS SDL backend
      -> WebAssembly SDL/Emscripten backend
      -> Samsung NaCl PPAPI backend
```

The runtime should expose a small API shaped more like raylib than SDL:

```text
LoadTexture / DrawTextureRegion / DrawSprite
IsKeyDown / WasKeyPressed
PlaySound
BeginFrame / EndFrame
```

But internally it should preserve SDL-like backend boundaries.

## Current Decision

For old Samsung NaCl, use a custom Pepper `Graphics3D` / GLES2 backend rather
than SDL's historical NaCl renderer. This is now measured on-device and is fast
enough to build on. SDL remains the preferred shape for local macOS and future
WebAssembly backends, while the game-facing API should stay small and
raylib-like.

The first reusable implementation is `engine/src/gles2_renderer.*`:

- batches colored rectangles and texture regions;
- supports nearest-filtered textures for pixel art;
- reports draw-call/quad/upload counters;
- includes a small `FrameMetrics` helper for one-second FPS windows.

## First Experiment

Do not refactor Tetris or Arkanoid first. Start with
`tizen-benchmark`:

1. Add a moving-ball/sprite benchmark.
2. Implement the same benchmark through:
   - current PPAPI 2D path;
   - optimized software framebuffer blit path;
   - macOS SDL renderer;
   - later, Emscripten SDL/WebAssembly.
3. Send metrics to the Node.js metrics server:
   - frame time;
   - min/median/p95/max;
   - dropped/late frames;
   - sprite count;
   - full-frame vs dirty-region mode.
4. Pick the backend based on TV metrics, not taste.

## Source Notes

- SDL homepage: https://www.libsdl.org/
- SDL Emscripten notes: https://wiki.libsdl.org/SDL2/README-emscripten
- Emscripten runtime environment: https://emscripten.org/docs/porting/emscripten-runtime-environment.html
- Emscripten SDL2 port setting: https://emscripten.org/docs/tools_reference/settings_reference.html
- Emscripten project builds and ports: https://emscripten.org/docs/compiling/Building-Projects.html
- raylib cheatsheet: https://www.raylib.com/cheatsheet/cheatsheet.html
- raylib examples: https://www.raylib.com/examples.html
- Historical SDL NaCl backend discussion: https://discourse.libsdl.org/t/native-client-backend-for-sdl2/18826
