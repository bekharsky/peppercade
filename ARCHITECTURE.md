# Peppercade Architecture

Peppercade has three strict ownership layers: reusable engine code, independent
games, and platform shells. Development probes and performance experiments sit
outside the product dependency graph.

## Dependency direction

```text
platforms/sdl ─┐
platforms/tv  ─┼──> games/* ───> engine
tizen-benchmark ─┘                ▲
```

- `engine/` must not include files from `games/`, `platforms/`, or tools.
- `games/` may include `engine/` headers but must not include SDL, PPAPI,
  JavaScript, or TV package APIs.
- `platforms/` owns windows, contexts, frame scheduling, device input, audio
  output, packaging, and game registration.
- `tizen-benchmark/` may exercise the engine directly but is never a game
  dependency.

All cross-layer includes use repository-root paths and every compiler receives
the repository root as an include directory. Moving a target therefore does not
change the dependency model or require fragile chains of `../` paths.

## Engine

The engine exposes the platform-neutral `Game` contract, `GameCatalog`, hub,
fixed-step loop, action-based input, GLES2 renderer, generated ASCII sprites,
shared arcade styling, HUD, particles, collision helpers, and generated audio.

Dynamic game scenes render through `Gles2Renderer`. `PixelCanvas` and
`StaticFrameCache` remain compatibility helpers for static or CPU-generated
content; new moving scenes should not depend on them.

Games emit `ArcadeSfx` events. Platform shells consume those events through the
SDL or NaCl audio backend, keeping device APIs out of game state.

## Games

Each directory under `games/` is named after its public Peppercade title and
owns its rules, state, assets, and optional standalone TV wrapper. Legacy class
names such as `TetrisGame` or `ArkanoidGlesGame` are implementation details and
must not leak into launcher labels, package display names, or directory names.

Each game implements `Game` and accepts only `InputState`, elapsed time, and the
shared renderer. The catalog is the single source of truth for IDs and labels;
platform shells register instances but do not duplicate game lifecycle logic.

Dragoban additionally owns its third-party level source. The generator reads
and writes only inside `games/dragoban`, while the repository-level notice
records redistribution terms.

## Platforms and targets

The SDL shell is shared by macOS and WebAssembly. It owns the desktop window,
WebGL/GL context, selector, keyboard mapping, and SDL audio. The TV shell owns
Pepper/PPAPI initialization, Graphics3D, remote input, NaCl audio, and the
package payload.

Supported build products are:

- macOS SDL application bundle;
- browser HTML/JavaScript/WebAssembly bundle;
- GitHub Pages static bundle;
- TV NaCl binaries for the shared pack, Stackfall, Brickbound, and the Tizen
  benchmark;
- signed TV packages for the shared pack, standalone games, and the Tizen
  benchmark.

All machine-specific tool locations and deployment details come from `.env`.
Tracked scripts contain no personal filesystem paths.

## Adding a game

1. Create `games/<public-name>/src` and optional `assets`.
2. Implement the `Game` interface using only engine and standard-library APIs.
3. Add the ID and public label to `engine/src/game_catalog.h`.
4. Register one instance in both platform shells.
5. Add the source files to the SDL/WebAssembly and TV build manifests.
6. Run `./scripts/build-all.sh` before publishing the change.

Shared math, color, random, HUD, context, timing, and audio behavior belongs in
the engine. Platform-specific behavior belongs in a shell. Neither should be
copied into an individual game.
