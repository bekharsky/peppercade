# Stackfall

Stackfall is the falling-block game used by the shared Peppercade pack and by a
standalone TV package. Game rules and rendering live in `src/stackfall_core.*`;
`src/stackfall.cc` is only the standalone NaCl wrapper.

Build the standalone binary with `./build_nacl.sh`, or build and package every
target from the repository root with `./scripts/build-all.sh`.

The TV application id is `tvTetris01.Stackfall`.
