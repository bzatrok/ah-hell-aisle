# AH: Hell Aisle — model bake-off

Four models each build the same game, independently, from the same spec, so we can
compare them. This repo is the arena, not a codebase.

## Layout

| Path | What it is |
|---|---|
| `SPEC.md` | The contract. The game every competitor must build. |
| `HANDOVER.md` | The exact prompt pasted into each model. Identical for all four. |
| `JUDGING.md` | Score sheet. |
| `assets/` | **Frozen.** Shared, identical art. Read-only to everyone. |
| `tools/gen_assets.py` | Regenerates `assets/` deterministically. Already run. |
| `opencode/` `vibe/` `claude/` `codex/` | One competitor each. Started empty. |

## Rules of the arena

- **Never edit `assets/`.** The whole comparison rests on the art being held constant.
- **Never touch another competitor's folder.** If you are working as one of the four,
  your world is your own folder plus read-only `../assets/`.
- **Do not "help" a competitor after the fact.** They each got one shot at
  `HANDOVER.md` with no coaching. Fixing one of them invalidates the run.
- The spec is C++17 + raylib + CMake, macOS. No other dependencies.

## If you are asked to work on the meta-repo

Changes to `SPEC.md` after the models have started invalidate the comparison. If the
spec is wrong, say so — don't quietly patch it.

## Prereqs

```sh
brew install cmake raylib
```
