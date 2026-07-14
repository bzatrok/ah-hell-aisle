# AH: Hell Aisle — claude_fable

A Doom 2 clone set in an Albert Heijn after closing time, per `../SPEC.md`.
C++17 + raylib + CMake, no other dependencies, every line of engine written for
this entry.

## Build & run

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

Run from this folder root so the game finds `../assets/` (there is a fallback
that resolves the assets folder relative to the binary if you run it from
somewhere else). Prereq: `brew install cmake raylib`.

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move / strafe |
| Mouse X or `←` `→` | Turn |
| Left mouse or `Ctrl` | Fire (hold for autofire, rate-limited) |
| `1` / `2` | Stokbrood / Prijspistool |
| `E` | Use doors (bumping into them also works) |
| `R` | Restart on the death or victory screen |
| `Esc` | Quit |

## What's here

Everything in SPEC.md §10, walkable end to end:

- **Renderer** — raylib 3D camera (yaw only) over a 44x40 tile grid extruded
  into textured walls, per-vertex distance shading Doom-style, billboarded
  sprites that depth-sort and get occluded by walls. The keycard door slides
  up like a proper Doom door.
- **Zones** — checkouts at the entrance, a gondola-aisle maze with long
  sightlines, a freezer section lit dim blue with a flickering strip light
  (and the keycard at the far end of it, guarded), and the magazijn behind the
  keycard door with the loading-dock exit.
- **Enemies** — 21 total: 8 winkelwagens (straight-line chargers), 8
  vakkenvullers (keep distance, lob dodgeable soup cans in an arc, blocked by
  walls), 5 zelfscanners (anchored hitscan turrets firing 3-shot bursts with
  visible beams). All have idle/notice (line of sight), chase, attack, death
  animations; corpses don't block movement or absorb shots.
- **Weapons** — stokbrood (25 dmg, 1.5u cone, 2/sec, damage lands on the
  impact frame) and prijspistool (20 dmg hitscan with small spread, 4/sec,
  labels as ammo, blocked by walls). Both play their 3-frame animations, both
  rate-limited, weapon bobs while walking.
- **HUD** — the shipped panel with HEALTH / AMMO / ARMOUR / KEY in its four
  cells, plus the face (ok / hurt below 50% / dead). Armour absorbs 50% of
  damage until depleted. Pickups at full value are not consumed.
- **States** — title, playing, GESLOTEN death screen, ONTSNAPT victory screen,
  both with time + kills stat line and `R` to restart without relaunching.
- **Sound** — synthesised at load time with raylib's audio API (no files):
  shots, swings, hits, deaths, pickups, the locked-door buzz, the roller door.

## Code layout

```
src/game.h     types + every SPEC-fixed number in one cfg namespace
src/level.cpp  the hand-authored map as ASCII art, parsed at load
src/game.cpp   simulation: movement, collision, weapons, enemy AI, doors, pickups
src/render.cpp 3D view: wall/floor/ceiling batching, shading, billboards
src/hud.cpp    weapon sprite, status bar, overlays, title/end screens
src/assets.cpp asset loading + sound synthesis
src/main.cpp   window + state machine
```

Plain structs and free functions — no ECS, no scripting, no abstraction the
game doesn't need (SPEC §11).

## Cut / limitations (honest list)

- **Enemy pathfinding is deliberately crude** — chase-toward-player with
  wall-slide and periodic re-aiming, as the spec invites. Enemies can get
  briefly confused behind gondola corners; they reacquire when you show
  yourself.
- **The soup can's arc is cosmetic** — it flies a visual parabola but its hit
  test is 2D on the ground plane, exactly like the rest of the combat.
- **Melee hits one target** per swing (the nearest in the cone), not a sweep.
- **Audio is minimal** — mono synthesised bleeps and noise bursts, functional
  rather than pretty. No music.
- **Tracers are plain 3D lines**, not sprites; muzzle flashes don't light the
  world.
- **No door-jamb side geometry polish** — doors are full-cube tiles; the spec's
  "sliding animation is a nice-to-have" IS implemented (doors rise), but the
  exit door doesn't animate (touching it ends the level immediately).
- Freezer "lighting" is per-vertex tint/flicker, not dynamic lights — it looks
  the part but nothing casts shadows.
