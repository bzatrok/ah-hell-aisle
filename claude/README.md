# AH: Hell Aisle — claude

A Doom 2 clone set in an Albert Heijn after closing time. C++17, raylib, CMake, no
other dependencies. Written from `../SPEC.md`, using the generated art in
`../assets/`; expanded after the bake-off to three levels, four weapons and five
enemies (handover 007). A WASM build lives in `../web/` (`../web/build.sh`) and
deploys to Cloudflare Pages.

## Build and run

From this folder:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

Needs `brew install cmake raylib`. The binary looks for `../assets/`, so run it from
this folder as shown. Builds clean under `-Wall -Wextra`.

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move / strafe |
| Mouse X, or arrow keys | Turn |
| Left mouse, or `Ctrl` | Swing / fire |
| `1` | Stokbrood — baguette, melee, 25 damage, infinite |
| `2` | Prijspistool — label gun, hitscan, 20 damage, uses labels |
| `3` | Statiegeldkanon — 7 pellets of deposit glass, uses flessen (find it first) |
| `4` | Vuurwerkpijl — rocket, 80 at the centre, hurts you too (find it first) |
| `E` | Open the magazijn door |
| `M` | Music on / off |
| `Esc` | Quit (desktop) — on the web build it returns to the title |
| `R` | On the death screen: this level again, with what you walked in carrying |

Three buildings between you and the car park: the winkel, the distributiecentrum,
the laadperron. Each ends at a loading dock door behind a keycard; on the last one
the pass is in De Bedrijfsleider's pocket, and he is not done with his shift.
Health, armour, ammo and weapons carry across levels. Kills and the clock score
the whole run.

## What's in it

- **Renderer.** A 40x40 tile grid extruded to full-height walls, a yaw-only camera,
  and a billboard for everything that moves. The store is baked into eight static
  meshes — one per wall texture, plus floor and ceiling — so the whole level draws in
  about eight calls. Sprites are alpha-discard cutouts, so they write real depth and
  occlude each other and the shelves correctly; they are sorted back-to-front as well.
- **Light.** Each zone's ambient colour is baked into the vertex colours when the mesh
  is built: cold blue in the freezer, sodium yellow in the magazijn, dead fluorescent
  in the aisles, and about one ceiling light in seven simply out. Two things are not
  baked, and they are why there is a shader at all — distance fog, which is what makes
  the far end of an aisle somewhere you have to walk to, and the freezer's failing
  strip lights, which flicker. Firing lights the aisle around you.
- **Five enemies that want different things.** The winkelwagen charges the moment it
  sees you and does not stop. The vakkenvuller holds about seven metres, sidesteps, and
  lobs soup cans in an arc you can walk out of — real projectiles, stopped by shelves.
  The zelfscanner barely moves, paints you with a frozen targeting line for a full
  second, then burst-fires down that line — step out of it and the burst misses. The
  beveiliger is that mechanic on legs: he closes to mid range and takes aimed single
  shots. De Bedrijfsleider, at the end, alternates between charging you down and
  standing to hurl fans of soup cans; the last keycard drops where he does.
- **Three levels.** Hand-authored as ASCII in `src/map.cpp`. The winkel: checkouts,
  gondola aisles, a freezer with the keycard in its far corner and a turret already
  looking at the door. The distributiecentrum: long parallel racking where every lane
  is a firing line. The laadperron: cold cells and a steel tangle under the boss
  arena. Loadout carries through each dock door; the geometry rebakes per level.
- **Sound and music.** Synthesised in code at startup: the asset pack ships no audio
  and the spec forbids fetching any. Two dozen oscillator-and-noise sounds — every
  enemy barks once when it spots you — plus a refrigeration hum, and a tiny 16-step
  sequencer that renders one bass-kick-hat loop per level (sparse, driving, fast) and
  a title drone. `M` mutes the music and leaves the shop noises alone.

## Code

Ten small modules, no framework:

| File | What it owns |
|---|---|
| `map.*` | Tiles, the three hand-authored levels, doors, line of sight, sliding |
| `player.*` | Input, movement, the four weapons, loadout carry-over, the keycard door |
| `enemy.*` | The five behaviours, the soup can and the rocket |
| `pickup.*` | The nine pickups |
| `world.*` | The bag of everything, and the order things happen in a frame |
| `render.*` | Meshes, shaders, billboards, the weapon sprite |
| `hud.*` | Status bar, messages, title / death / victory screens |
| `assets.*` `audio.*` `game.*` `main.cpp` | Textures, synthesised sound, state machine, entry point |

No ECS, no level editor, no test suite, no scripting layer. SPEC §11 asks for none of
them, and this is a game rather than a framework.

## What I cut, and why

Honestly:

- **Melee and hitscan resolve on the frame you press the button**, not on the impact
  frame of the three-frame animation, so a hit lands slightly "early" against the art.
  I chose responsiveness: a swing that connects 150ms after the click feels broken even
  when it is the more truthful thing to do.
- **Enemies do not path.** They walk at you and slide along walls, which is what the
  spec allows and I did not exceed it. So a vakkenvuller can end up shuffling behind
  the corner of a gondola rather than walking around it. I flip its sidestep direction
  when it stalls, which hides most of that, but not all of it. A navmesh was explicitly
  the wrong thing to build here.
- **The loading dock door never opens.** Touching it ends the level, which is what the
  spec asks for, so it is a door that is only ever scenery. The magazijn door does
  slide up properly.
- **No mipmaps.** Point filtering throughout, so distant textures shimmer a little.
  That is the retro look I was after, but it is a trade and not a free one.
- **Fixed 1280x720 window, not resizable.** The status bar's four cells are positioned
  in screen pixels against that width. Resolution independence is work the spec did not
  ask for.
- **Enemies have no facing sprites.** The art ships no rotation sheet — as the manifest
  says it does not — so a monster looks the same coming and going.
- **The sound is unscored, and it shows.** It is oscillators. It is not good. It is,
  however, mine, and an Albert Heijn at 3am does not sound good either.

One note if you read `render.cpp`: raylib's `DrawBillboardPro` anchors the **bottom**
edge of the quad at the position you hand it when the origin is zero — it does not
centre on it. Every frame of the art is drawn feet-to-the-bottom-row, corpses included,
so passing the floor position is exactly right, and a corpse ends up lying on the floor
for free. I found that out the way everyone finds that out.
