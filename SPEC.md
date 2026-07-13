# AH: Hell Aisle — specification

A Doom 2 clone set in an Albert Heijn after closing time.

This document is the **contract**. Four models each implement it independently, in
their own folder, from an empty directory. Same spec, same assets, same build
target — the only variable is the model. Read it end to end before writing code.

---

## 1. Premise

You are a night-shift **vakkenvuller**. The store closed at 22:00. At 02:14 something
came up through the drain under the fresh-fish counter, and the shop is no longer a
shop. Find the bedrijfsleider's keycard, get into the magazijn, and get out through
the loading dock.

Tone: straight-faced Doom brutality applied to Dutch supermarket banality. Play it
completely seriously. That is the joke.

---

## 2. Hard constraints

Non-negotiable. A submission that breaks any of these is disqualified.

| | |
|---|---|
| **Language** | C++17. No C++20/23 features. |
| **Renderer / platform lib** | **raylib**, and nothing else. |
| **Build** | CMake. Must configure and build on macOS (Apple Silicon) with raylib installed via Homebrew. |
| **Third-party deps** | **None beyond raylib.** No engines, no ECS libraries, no physics libraries, no header-only grab-bags. The C++17 standard library is fine. |
| **Assets** | Load read-only from `../assets/`. Do **not** edit, add to, regenerate, or reach outside that folder. See `assets/MANIFEST.md`. |
| **Scope of your work** | Your folder only. Never read or write another competitor's folder. |
| **Code origin** | Every line of the engine is yours. Do not vendor a raycaster, a Doom port, or a tutorial project. |

Assume the grader runs exactly this, from inside your folder:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

That must work with no further arguments, no environment variables, and no manual
steps. The binary must be named `ah_hell_aisle` and must find `../assets/` when run
from your folder root as shown.

---

## 3. Rendering model — this is a Doom clone, not a Quake clone

- **World:** a 2D **grid** of tiles, extruded into 3D walls of uniform height. Floor
  and ceiling are flat and textured. No rooms-over-rooms, no sloped floors, no
  vertical variation. A tile is either empty or a solid wall of a given texture.
- **Camera:** raylib 3D camera at eye height, **yaw only**. No pitch, no free-look,
  no vertical aim — shots are resolved in 2D on the horizontal plane, exactly like
  Doom. No jumping, no crouching.
- **Enemies, pickups, projectiles:** flat 2D **billboard sprites** that always face
  the camera. Never 3D models.
- **Weapon:** a 2D sprite drawn in screen space, pinned to the bottom of the screen,
  bobbing while you walk.
- **Depth:** sprites must sort and occlude correctly against walls and each other. A
  monster behind a shelf is hidden by the shelf.

You may use raylib's 3D camera and its billboard helpers, or write your own
projection. Both are in the spirit of the exercise. What you may not do is make it
a true-3D-model game.

---

## 4. Player

| | |
|---|---|
| Health | 100, max 100 (pickups cannot exceed max) |
| Armour | 0, max 100. Absorbs **50%** of incoming damage until depleted. |
| Move speed | ~3.5 world-units/sec; strafe same; no acceleration curve required |
| Turn speed | ~120°/sec on keyboard; mouse look on the X axis |
| Collision | Circle-vs-tile against walls. You must not be able to clip through a wall, walk into a solid tile, or escape the map. |
| Death | Health ≤ 0 → death state → a "YOU DIED" / *"GESLOTEN"* screen with a restart key. |

### Controls (mandatory — the grader will use exactly these)

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move forward / strafe left / back / strafe right |
| Mouse X, or `←` `→` | Turn |
| Left mouse, or `Ctrl` | Fire |
| `1` | Select Stokbrood |
| `2` | Select Prijspistool |
| `E` | Use (open doors) |
| `Esc` | Quit |
| `R` | Restart, on the death or victory screen |

Mouse must be captured/hidden during play.

---

## 5. Weapons

Two, both mandatory. Slot 1 is available from the start; so is slot 2, with 40 label
ammo.

### 1 — Stokbrood (melee)

A day-old baguette, swung like a bat. Infinite use.

| | |
|---|---|
| Damage | 25 |
| Range | 1.5 world-units, in a narrow cone directly ahead |
| Rate | ~2 swings/sec |
| Ammo | none |

### 2 — Prijspistool (hitscan)

A price-label gun. Fires a sticky label at the speed of retail.

| | |
|---|---|
| Damage | 20 |
| Range | effectively unlimited (blocked by walls) |
| Rate | ~4 shots/sec |
| Ammo | 1 label per shot. Start with 40, cap 200. |
| Spread | small random spread is encouraged, not required |

Hitscan must be blocked by walls: you cannot shoot a monster through a shelf.
Both weapons must play their 3-frame animation and must not fire faster than their
rate by spamming the key.

---

## 6. Enemies

Three, all mandatory, all behaviourally distinct. Each must: idle until it notices
you, chase or engage, attack, take damage, die through its death animation, and then
stop being a threat (corpses do not block movement or absorb shots).

| | Winkelwagen | Vakkenvuller | Zelfscanner |
|---|---|---|---|
| **Sprite** | `enemy_winkelwagen.png` | `enemy_vakkenvuller.png` | `enemy_zelfscanner.png` |
| **Health** | 30 | 60 | 100 |
| **Speed** | 4.0 (faster than you) | 1.8 (slower than you) | 0.4 (barely moves) |
| **Attack** | Melee. 10 damage on contact. | Ranged. Throws a soup can (`proj_soepblik.png`), a slow visible projectile, 15 damage on hit. | Hitscan. 8 damage, fires in bursts, must have line of sight. |
| **Range** | contact | ~10 units | ~14 units |
| **Cooldown** | ~1.0s | ~2.0s | ~1.5s between bursts |
| **Behaviour** | Charges in a straight line the moment it sees you. Low HP, high threat. The rusher. | Keeps its distance and lobs cans in an arc you can sidestep. The zoner. | Anchors a position and denies it with a beam. Punishes standing in the open. The turret. |

Requirements:
- Enemies must not walk through walls or through each other.
- Enemies must only engage with **line of sight** to the player.
- Soup cans are real projectiles: they travel, they can be dodged, and they are
  stopped by walls.
- At least **20 enemies** across the level, with a mix of all three types.

A crude line-of-sight raycast and a "walk toward the player, slide along walls"
chase is entirely acceptable. Do not build a navmesh.

---

## 7. Pickups

Billboarded, picked up by walking over them, and they play a pickup effect (a flash,
a HUD message, a sound — your choice).

| Sprite | Effect |
|---|---|
| `pickup_appelflap.png` | +25 health, capped at 100 |
| `pickup_rookworst.png` | +50 health, capped at 100 |
| `pickup_labels.png` | +20 label ammo, capped at 200 |
| `pickup_bonuskaart.png` | +50 armour, capped at 100 |
| `pickup_keycard.png` | The keycard. Exactly one in the level. |

Pickups at full value (e.g. health at 100) must not be consumed.

---

## 8. The level

**One** level, hand-authored by you as a tile grid, at least **32x32** tiles. It must
be a store you can get lost in for a couple of minutes — not a corridor.

Mandatory zones, each visually distinct through its wall textures:

1. **Entrance / checkouts** — where you start. `wall_checkout`, `wall_plain`.
2. **The aisles** — the bulk of the map. A maze of `wall_shelf_full` /
   `wall_shelf_empty` gondolas with sightlines down each aisle.
3. **The freezer section** — `wall_freezer`. Noticeably darker than the rest of the
   store. Do something with the lighting here.
4. **Back-of-house / magazijn** — `wall_magazijn`, behind the locked door.

Mandatory progression:

- The **keycard** is somewhere in the store proper, and getting it must be
  defended — it is not lying next to your spawn.
- The **`door_keycard`** door gates the magazijn. Without the card it does not open;
  bumping it or pressing `E` on it tells the player they need the pass. With the
  card, `E` (or contact) opens it.
- The **`door_exit`** door is in the magazijn. Touching it wins the level.

Doors are tiles. A simple "locked / unlocked / open" state per door tile is enough;
sliding animation is a nice-to-have, not a requirement.

---

## 9. HUD and game states

Status bar across the bottom, built from `hud_panel.png` and `hud_face.png`:

- **HEALTH** as a percentage.
- **AMMO** — current label count.
- **ARMOUR** as a percentage.
- **KEY** — shows the keycard icon once you have it.
- **FACE** — frame 0 above 50% health, frame 1 below 50%, frame 2 when dead.

Three states, all required:

1. **Title** — game name, "press any key". Doesn't need to be fancy.
2. **Playing**.
3. **End** — either death (*GESLOTEN*) or victory (you reached the loading dock).
   Show a stat line: time taken, kills / total, and a restart key.

---

## 10. Definition of done

Your folder is complete when a grader can, from a clean checkout:

- [ ] Run the three build commands in §2 and get a running game, first try, no warnings-as-errors surprises.
- [ ] Read a `README.md` in your folder: what you built, how to build it, controls, and anything you cut and why.
- [ ] Walk around a store with textured walls, a bobbing weapon, and a working status bar.
- [ ] Get killed by each of the three enemy types.
- [ ] Kill each of the three enemy types with each of the two weapons.
- [ ] Fail to open the magazijn door, find the keycard, open the magazijn door.
- [ ] Reach the loading dock and see a victory screen.
- [ ] Die and see a death screen.
- [ ] Restart from either end screen without relaunching the binary.

Your folder must contain: `CMakeLists.txt`, `README.md`, and your sources. Nothing
generated (no `build/`) — add a `.gitignore`.

---

## 11. Non-goals — do not build these

Actively out of scope. Building them costs you points; they are complexity that
this spec does not want.

- Multiple levels, or any level loading from disk. One hard-coded level.
- Save/load. Difficulty settings. Options menus. Key rebinding.
- Multiplayer, networking, controller support.
- Physics beyond "circle slides along wall".
- An entity-component-system, a scripting layer, a plugin architecture, or a level
  editor.
- Unit tests. This is a game; the acceptance test is §10 and it is done by hand.
- Vertical aim, jumping, crouching, mouse-look on Y.
- Downloading anything.

---

## 12. How you will be judged

See `JUDGING.md`. In short, in descending weight: **does it build**, **does it run
without crashing**, **is it faithful to this spec**, **is it fun for ninety
seconds**, and **would you want to maintain this code**.

Speed is not a criterion. Take the time to get it right.
