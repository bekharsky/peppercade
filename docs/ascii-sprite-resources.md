# ASCII Sprite Resources

Peppercade sprite grids use the versioned `.pascii` resource format. These
files are the source of truth; generated `*_assets.h` files must not be edited
by hand.

## Format

One resource can contain any number of named sprites, and every sprite declares
its own resolution. Low- and high-resolution variants can therefore coexist in
the same file and be selected independently by game code.

```text
pascii 1
namespace example_assets
output example/src/example_assets.h

sprite kBallLow
size 8 8
padding 1 1
anchor 0.5 0.5
blend alpha
pixels
|  ....  |
| .^^^^. |
|.^!!!!^.|
|.^====^.|
|.^====^.|
|.^::::^.|
| .____. |
|  ....  |
end
```

The `|...|` delimiters preserve leading and trailing transparent cells without
creating trailing whitespace in Git. Rows are preserved exactly. The parser
rejects an incorrect row count, incorrect width, unknown symbol, duplicate
sprite, invalid output path, or an entirely transparent sprite.

`padding` is expressed in virtual cells. `anchor` is normalized from `0` to `1`
and is used by `DrawAsciiSpriteAt`; bounds-based drawing remains available when
the caller deliberately wants to stretch a grid into a rectangle. `blend` is
either `opaque` or `alpha`.

## Ink alphabet

The characters describe material roles rather than fixed colors. A game
supplies the colors through `MakeAsciiSpritePalette`.

| Character | Ink role |
| --- | --- |
| space | transparent |
| `.` | outline |
| `_` | deep shadow |
| `-` | shadow |
| `:` | dark base tone |
| `=` | base tone |
| `^` | light tone |
| `*` | highlight |
| `!` | specular highlight |
| `@` | accent |
| `%` | light accent |
| `~` | soft glow |
| `+` | glow |
| `#` | hot glow |

The expanded ramp keeps simple sprites concise while allowing glass, neon,
metal, and multi-band shaded objects without hard-coding colors into geometry.

## Generate and validate

```sh
python3 scripts/ascii_resources.py
python3 scripts/ascii_resources.py --check
python3 scripts/test_ascii_resources.py
```

The canonical serializer makes editor-to-text-to-editor round trips lossless.
Generated headers contain horizontal runs, not individual cells, which greatly
reduces the number of renderer quads for solid areas.
