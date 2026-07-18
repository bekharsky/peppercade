#!/usr/bin/env python3
"""Validate Peppercade .pascii resources and generate optimized C++ headers."""

from __future__ import annotations

import argparse
import dataclasses
import difflib
import pathlib
import re
import sys
from typing import Iterable


INKS = {
    ".": "kAsciiOutline",
    "_": "kAsciiDeepShadow",
    "-": "kAsciiShadow",
    ":": "kAsciiDark",
    "=": "kAsciiBase",
    "^": "kAsciiLight",
    "*": "kAsciiHighlight",
    "!": "kAsciiSpecular",
    "@": "kAsciiAccent",
    "%": "kAsciiAccentLight",
    "~": "kAsciiSoftGlow",
    "+": "kAsciiGlow",
    "#": "kAsciiHotGlow",
}


class ResourceError(ValueError):
    pass


@dataclasses.dataclass(frozen=True)
class Sprite:
    name: str
    width: int
    height: int
    padding_x: int
    padding_y: int
    anchor_x: float
    anchor_y: float
    blend: str
    pixels: tuple[str, ...]


@dataclasses.dataclass(frozen=True)
class Resource:
    namespace: str
    output: str
    sprites: tuple[Sprite, ...]


def fail(path: pathlib.Path, line: int, message: str) -> ResourceError:
    return ResourceError(f"{path}:{line}: {message}")


def parse_resource(path: pathlib.Path) -> Resource:
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0] != "pascii 1":
        raise fail(path, 1, "expected 'pascii 1'")

    namespace = ""
    output = ""
    sprites: list[Sprite] = []
    i = 1
    while i < len(lines):
        raw = lines[i]
        line_number = i + 1
        i += 1
        if not raw or raw.startswith(";"):
            continue
        key, separator, value = raw.partition(" ")
        if not separator:
            raise fail(path, line_number, f"expected directive, got {raw!r}")
        if key == "namespace":
            if namespace:
                raise fail(path, line_number, "duplicate namespace")
            if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", value):
                raise fail(path, line_number, "namespace must be a C++ identifier")
            namespace = value
        elif key == "output":
            if output:
                raise fail(path, line_number, "duplicate output")
            output = value
        elif key == "sprite":
            sprite, i = _parse_sprite(path, lines, i, value, line_number)
            sprites.append(sprite)
        else:
            raise fail(path, line_number, f"unknown resource directive {key!r}")

    if not namespace:
        raise fail(path, 1, "missing namespace")
    if not output:
        raise fail(path, 1, "missing output")
    output_path = pathlib.PurePosixPath(output)
    if (not output.endswith(".h") or output_path.is_absolute() or
            ".." in output_path.parts):
        raise fail(path, 1, "output must be a relative .h path")
    if not sprites:
        raise fail(path, 1, "resource contains no sprites")
    names = [sprite.name for sprite in sprites]
    if len(names) != len(set(names)):
        raise fail(path, 1, "sprite names must be unique")
    return Resource(namespace, output, tuple(sprites))


def _parse_sprite(path: pathlib.Path, lines: list[str], i: int, name: str,
                  start_line: int) -> tuple[Sprite, int]:
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        raise fail(path, start_line, "sprite name must be a C++ identifier")
    width = height = None
    padding_x = padding_y = 0
    anchor_x = anchor_y = 0.5
    blend = "opaque"
    seen: set[str] = set()

    while i < len(lines):
        raw = lines[i]
        line_number = i + 1
        i += 1
        if not raw or raw.startswith(";"):
            continue
        if raw == "pixels":
            if width is None or height is None:
                raise fail(path, line_number, "size must precede pixels")
            if i + height > len(lines):
                raise fail(path, line_number, "not enough pixel rows")
            pixel_lines = lines[i:i + height]
            if not all(row.startswith("|") and row.endswith("|")
                       for row in pixel_lines):
                raise fail(path, line_number + 1,
                           "every pixel row must use |...| framing")
            pixels = tuple(row[1:-1] for row in pixel_lines)
            i += height
            if i >= len(lines) or lines[i] != "end":
                raise fail(path, i + 1, "expected 'end' after pixel rows")
            i += 1
            _validate_pixels(path, line_number + 1, pixels, width)
            return Sprite(name, width, height, padding_x, padding_y,
                          anchor_x, anchor_y, blend, pixels), i

        key, separator, value = raw.partition(" ")
        if not separator or key not in {"size", "padding", "anchor", "blend"}:
            raise fail(path, line_number, f"unknown sprite directive {raw!r}")
        if key in seen:
            raise fail(path, line_number, f"duplicate {key}")
        seen.add(key)
        parts = value.split()
        try:
            if key == "size":
                if len(parts) != 2:
                    raise ValueError
                width, height = (int(part) for part in parts)
                if width <= 0 or height <= 0 or width > 512 or height > 512:
                    raise fail(path, line_number, "size must be between 1 and 512")
            elif key == "padding":
                if len(parts) != 2:
                    raise ValueError
                padding_x, padding_y = (int(part) for part in parts)
                if padding_x < 0 or padding_y < 0:
                    raise fail(path, line_number, "padding cannot be negative")
            elif key == "anchor":
                if len(parts) != 2:
                    raise ValueError
                anchor_x, anchor_y = (float(part) for part in parts)
                if not (0.0 <= anchor_x <= 1.0 and 0.0 <= anchor_y <= 1.0):
                    raise fail(path, line_number, "anchor must be between 0 and 1")
            elif key == "blend":
                if value not in {"opaque", "alpha"}:
                    raise fail(path, line_number, "blend must be opaque or alpha")
                blend = value
        except ValueError:
            raise fail(path, line_number, f"invalid {key} value {value!r}")
    raise fail(path, start_line, f"sprite {name!r} has no pixels block")


def _validate_pixels(path: pathlib.Path, first_line: int,
                     pixels: tuple[str, ...], width: int) -> None:
    painted = False
    for offset, row in enumerate(pixels):
        if len(row) != width:
            raise fail(path, first_line + offset,
                       f"row width is {len(row)}, expected {width}")
        for column, symbol in enumerate(row, 1):
            if symbol != " " and symbol not in INKS:
                raise fail(path, first_line + offset,
                           f"unknown ink {symbol!r} at column {column}")
            if symbol in INKS:
                painted = True
    if not painted:
        raise fail(path, first_line, "sprite must contain at least one painted cell")


def serialize_resource(resource: Resource) -> str:
    out = ["pascii 1", f"namespace {resource.namespace}",
           f"output {resource.output}", ""]
    for index, sprite in enumerate(resource.sprites):
        out.extend([
            f"sprite {sprite.name}",
            f"size {sprite.width} {sprite.height}",
            f"padding {sprite.padding_x} {sprite.padding_y}",
            f"anchor {_number(sprite.anchor_x)} {_number(sprite.anchor_y)}",
            f"blend {sprite.blend}",
            "pixels",
            *(f"|{row}|" for row in sprite.pixels),
            "end",
        ])
        if index + 1 < len(resource.sprites):
            out.append("")
    return "\n".join(out) + "\n"


def _number(value: float) -> str:
    return f"{value:.6f}".rstrip("0").rstrip(".")


def _runs(sprite: Sprite) -> Iterable[tuple[int, int, int, str]]:
    for y, row in enumerate(sprite.pixels):
        x = 0
        while x < sprite.width:
            symbol = row[x]
            if symbol == " ":
                x += 1
                continue
            end = x + 1
            while end < sprite.width and row[end] == symbol:
                end += 1
            yield x, y, end - x, INKS[symbol]
            x = end


def generate_header(resource_path: pathlib.Path, resource: Resource) -> str:
    guard = re.sub(r"[^A-Za-z0-9]", "_", resource.output.upper()) + "_"
    source = resource_path.as_posix()
    include = "engine/src/ascii_sprite.h"
    out = [
        f"// Generated by scripts/ascii_resources.py from {source}.",
        "// Edit the .pascii resource, not this file.",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        f'#include "{include}"',
        "",
        f"namespace {resource.namespace} {{",
    ]
    for sprite in resource.sprites:
        out.append("")
        out.append(f"static const AsciiSpriteRun {sprite.name}Runs[] = {{")
        for x, y, length, ink in _runs(sprite):
            out.append(f"    {{{x}, {y}, {length}, {ink}}},")
        out.extend([
            "};",
            f"static const AsciiSprite {sprite.name} = {{",
            f"    {sprite.name}Runs,",
            f"    static_cast<int>(sizeof({sprite.name}Runs) / sizeof({sprite.name}Runs[0])),",
            f"    {sprite.height}, {sprite.width},",
            f"    {sprite.padding_x}, {sprite.padding_y},",
            f"    {_number(sprite.anchor_x)}f, {_number(sprite.anchor_y)}f,",
            f"    kAsciiBlend{sprite.blend.title()},",
            "};",
        ])
    out.extend(["", f"}}  // namespace {resource.namespace}", "",
                f"#endif  // {guard}", ""])
    return "\n".join(out)


def discover(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(root.glob("games/*/assets/*.pascii"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parent.parent)
    parser.add_argument("--check", action="store_true",
                        help="validate canonical resources and generated headers")
    parser.add_argument("--format", action="store_true",
                        help="rewrite resources into canonical form")
    args = parser.parse_args()
    root = args.root.resolve()
    failures = 0
    paths = discover(root)
    if not paths:
        print("No .pascii resources found", file=sys.stderr)
        return 1
    for path in paths:
        try:
            resource = parse_resource(path)
            canonical = serialize_resource(resource)
            original = path.read_text(encoding="utf-8")
            if args.format and original != canonical:
                path.write_text(canonical, encoding="utf-8")
            elif args.check and original != canonical:
                failures += 1
                print(f"{path}: resource is not canonical", file=sys.stderr)
            output = root / resource.output
            generated = generate_header(path.relative_to(root), resource)
            existing = output.read_text(encoding="utf-8") if output.exists() else ""
            if args.check:
                if existing != generated:
                    failures += 1
                    print(f"{output}: generated header is stale", file=sys.stderr)
                    diff = difflib.unified_diff(existing.splitlines(),
                                                generated.splitlines(),
                                                fromfile=str(output),
                                                tofile="generated", lineterm="")
                    print("\n".join(list(diff)[:80]), file=sys.stderr)
            elif existing != generated:
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_text(generated, encoding="utf-8")
        except (OSError, ResourceError) as error:
            failures += 1
            print(error, file=sys.stderr)
    if failures:
        return 1
    action = "Validated" if args.check else "Generated"
    print(f"{action} {len(paths)} ASCII resource file(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
