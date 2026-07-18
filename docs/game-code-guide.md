# Game Code Guide

This is the short maintenance map for Peppercade game and engine work. Read it
before introducing a helper in a game directory.

## Ownership

```text
Game class                 rules, state, level generation, game-specific layout
game.h / game_catalog      platform-neutral lifecycle and game identity
math_util.h                scalar min/max/clamp/abs/cyclic values
color_util.h               GlesColor mixing, shading, lightening, luminance
random_util.h              deterministic RNG step; each game owns its seed
arcade_style / arcade_hud  shared visual language, grid/HUD geometry, overlays
ascii_sprite               generated editable sprite rendering
pixel_effects              particles and bursts
arcade_audio               SFX ids, event masks, backend-agnostic playback loop
game_hub                   selector rendering shared by pack shells
nacl_*                     Pepper input, audio, clock, and GLES context setup
```

## Rules For New Code

1. Search `engine/src` before writing a local utility. In particular,
   never copy an LCG, color shading function, SFX bit loop, or NaCl context
   attribute list into a game.
2. Keep only rules with game-specific semantics in a game. Pause restrictions,
   difficulty side effects, collision rules, and level generation are expected
   to differ and should not be hidden behind a generic helper.
3. Use `arcade::ComputeGridSideLayout` for a fixed-cell grid next to the common
   HUD. Use a custom layout only when the board has a real extra constraint,
   such as 2048's square board or Brickbound's aspect ratio.
4. Queue sounds in game code, but play them only in a shell. Muting remains game
   state because the HUD displays it.
5. Keep RNG state on the game instance and call `arcade::NextRandom(&rng_)`.
   This preserves deterministic behavior across SDL, WebAssembly, and NaCl.
6. Draw in this order: background, panel/grid, pieces, border overlay, HUD,
   transient effects, pause/game-over overlay. Exceptions should be commented.

## Intentional Similarities

Not every similar-looking method is a duplicate. `SetPaused`,
`CycleDifficulty`, and `Reset` have different state guards and side effects in
each game. Their small wrappers make the `Game` interface explicit and should
remain local. Likewise, flood fills in Bubble Orbit, Lightline, and Dragoban
operate on different graph rules and should not share a vague generic BFS.

## Validation

For changes to shared headers or rendering helpers, run:

```sh
python3 scripts/test_ascii_resources.py
./platforms/sdl/build_macos_sdl.sh
./platforms/sdl/build_wasm.sh
./scripts/build-all-nacl.sh
git diff --check
```

Build scripts list `.cc` files explicitly. A new non-header-only engine module
must be added to every target that uses it.
