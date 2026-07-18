# Peppercade Visual Style

Peppercade games should look like a polished late-80s/early-90s TV arcade UI:
pixel-sharp geometry, restrained dark surfaces, soft muted decorative colors,
and glossy highlights on active pieces.

## Core Principles

- Pixel art first: hard edges, integer-aligned rectangles, no soft gradients.
- Gloss is allowed, but only as chunky pixel bands: bright top highlight and
  dark lower shadow.
- Backgrounds stay dark and quiet so moving objects read clearly on TV.
- HUD panels are flat dark panels with a single strong outline.
- Text is blocky 5x7 uppercase or seven-segment numeric display.
- Avoid one-hue themes. Use dark navy bases with cyan, mint, amber, rose, and
  saturated piece colors.

## Palette

```text
screen background       #050712
playfield background    #050914
grid line               #112c3d
board border            #ffd166
hud panel               #0d0d22
hud border              #a5849a
label text              #acc8c0
score/cyan display      #7de1cc
speed/amber display     #ffd884
overlay panel           #070c19
overlay text            #ebdeb8
game-over border        #c65c70
pause border            #cfb476
block shadow            #141626
block highlight         #ffffff
```

Piece/sprite accent colors inherited from Tetris:

```text
cyan      #24ddff
blue      #5776ff
orange    #ff9a21
yellow    #ffd854
green     #37e09b
purple    #be68ff
pink/red  #ff5272
```

## Text

- Labels use a 5x7 uppercase bitmap font.
- Letter spacing is one pixel cell at the selected scale.
- HUD labels use mint text.
- Overlay text uses cream text.
- Numeric score/speed displays use seven-segment blocks, not the 5x7 font.

## Blocks And Sprites

Glossy arcade blocks use this structure:

1. Fill the block with the saturated base color.
2. Add a white horizontal highlight near the top.
3. Add a dark navy horizontal shadow near the bottom.
4. Keep the silhouette square and pixel-aligned.

This should be used for Tetris pieces, Arkanoid bricks, pickups, and simple
button-like in-game objects.

## Panels

- Board/playfield border: amber.
- HUD border: dusty rose.
- Overlay border: amber for pause, red/rose for game over.
- HUD sections should breathe vertically; do not cram label/value pairs.
- Side HUD geometry and typography must come from `arcade_hud`:
  `ScreenMargin`, `SideHudGap`, `SideHudWidth`, and `ComputeHudLayout`.
  Games provide only the panel rectangle and maximum digit count, then append
  content through `HudFlow`. HUD blocks stack from top to bottom at their
  natural height plus the shared gap; they must not be distributed evenly over
  the panel or use local viewport-based sizing rules.

## Background Patterns

Use low-contrast geometric patterns in muted colors. Pattern colors may vary
between games, but they should not overpower the playfield. Prefer squares,
crosses, stepped bars, and outlined rectangles.
