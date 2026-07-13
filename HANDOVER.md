# Handover — build "AH: Hell Aisle"

**You are one of four models in a bake-off.** Each of us gets this exact file, one
shot, no follow-up coaching. Same spec, same art, same build target. The only
variable is you.

**Your folder** is the one named after you: `opencode/`, `vibe/`, `claude/`, or
`codex/`. The operator names it when handing you this file. Everything you write
goes in there and nowhere else. It currently holds nothing but a placeholder
`README.md` — overwrite it.

---

## What to build

A **Doom 2 clone set in an Albert Heijn after closing time**. You are a night-shift
vakkenvuller; the shop has turned; find the keycard, open the magazijn, escape
through the loading dock.

**Read `SPEC.md` in the repo root, end to end, before you write a line.** It is the
contract, and it is complete: exact enemy stats, weapon damage and rates, controls,
level requirements, the definition of done, and an explicit list of things *not* to
build. Do not deviate from it, do not "improve" it, and do not ask for
clarification — there is nobody to ask. Where it deliberately leaves room (level
layout, code architecture, feel), that room is yours and it is where you win or lose.

**Read `assets/MANIFEST.md` next.** It is the art contract: filenames, sizes, frame
layouts, sprite anchoring.

## The five things that will trip you up

1. **`assets/` is frozen.** Twenty-two PNGs, shared and identical across all four
   competitors, read-only. Load from `../assets/`. Never edit, add, regenerate, or
   substitute — the whole comparison rests on art being held constant. If a sprite
   seems wrong, work with it anyway.
2. **raylib and nothing else.** C++17, CMake, macOS. No engine, no ECS library, no
   physics library, no header-only grab-bags, no vendored raycaster. The standard
   library is fine. Every line of the engine is yours.
3. **It's Doom, not Quake.** A 2D tile grid extruded into flat-topped walls, a
   yaw-only camera, and 2D billboard sprites for every enemy, pickup and projectile.
   No 3D models, no vertical aim, no jumping. Sprites must depth-sort and be
   occluded by walls.
4. **Ship the whole checklist, not the pretty half.** §10 of `SPEC.md` is how you're
   graded and a grader will walk it by hand: build, play, get killed by each of the
   three enemies, kill each with each of the two weapons, be refused by the locked
   door, find the keycard, escape, die, restart. A gorgeous renderer with no keycard
   logic loses to an ugly game that completes.
5. **Read §11, the non-goals.** Building things the spec explicitly excludes — an
   ECS, a level editor, a test suite, multiple levels, a settings menu — costs you
   points. This is a game. Restraint is part of the grade.

## The build the grader will run

From inside your folder, exactly this, first try, no arguments, no env vars:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

The binary must be named `ah_hell_aisle`. Prereq already installed:
`brew install cmake raylib`.

## What your folder must contain when you're done

- `CMakeLists.txt` — yours, working, no vendored dependencies.
- `README.md` — what you built, how to build it, the controls, and an **honest** list
  of what you cut and why. The honesty is scored. A README claiming a feature that
  isn't there scores worse than one that admits it was dropped.
- `.gitignore` — at minimum `build/`.
- Your sources. Structured however you think is right; "would you want to maintain
  this" is a scored criterion, and so is not over-abstracting a small game.

Nothing generated. No `build/` committed.

## Rules of the arena

- Work only in your folder, plus read-only `../assets/`. **Never read or write
  another competitor's folder** — not to peek, not to compare, not to borrow.
- Do not modify `SPEC.md`, `JUDGING.md`, `CLAUDE.md`, `assets/`, or `tools/`.
- Do not commit. The operator handles git.
- Speed is not a criterion. Nobody is timing you. Get it right.

Good luck. Play it completely straight — the horror is real, the store is an Albert
Heijn, and that is the joke.
