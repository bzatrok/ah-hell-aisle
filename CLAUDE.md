# AH: Hell Aisle

A Doom 2 clone set in an Albert Heijn after closing time. Started as a four-model
bake-off (the contract lives on in `SPEC.md` / `HANDOVER.md` / `JUDGING.md`); the
competition was decided on 2026-07-14: **`claude/` won** and is now the game under
active development. The arena freeze on `claude/` is lifted.

## Layout

| Path | What it is |
|---|---|
| `claude/` | **The game.** The winning implementation, open for development. |
| `web/` | WASM harness + Cloudflare Pages deploy. `./web/build.sh` → `web/dist/`. |
| `assets/` | The art. Generated deterministically by `tools/gen_assets.py` — keep it regenerable. |
| `runner-ups/` | The other bake-off implementations, archived. |
| `SPEC.md` `HANDOVER.md` `JUDGING.md` | The original competition contract, kept for the record. |
| `.handovers/` | Handover docs and the log tracking them. |

## Rules

- **`runner-ups/` is a museum: never edit it.** (From their new depth the archived
  games find the art via their `../../assets/` fallback, where they have one.)
- New art goes through `tools/gen_assets.py`, never hand-dropped into `assets/`,
  so the whole set stays reproducible from one script.
- The spec is C++17 + raylib + CMake. The web build adds Emscripten, nothing else.

## Build

Native (from `claude/`):

```sh
brew install cmake raylib
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
./build/ah_hell_aisle
```

Web (from the repo root):

```sh
brew install emscripten
./web/build.sh   # → web/dist/, deployable to Cloudflare Pages project ah-hell-aisle
```
