# Brickbound

Brickbound is the brick-breaking game used by the shared Peppercade pack and by
a standalone TV package. `src/brickbound_gles_game.*` contains the
platform-neutral game; `src/brickbound_nacl.cc` is only the standalone wrapper.

Build the standalone binary with `./build_nacl.sh`, or build and package every
target from the repository root with `./scripts/build-all.sh`.

The TV application id is `tvArknd001.Brickbound`.
