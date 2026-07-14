# AH: Hell Aisle — claude

A Doom 2 clone set in an Albert Heijn after closing time. C++17, raylib, CMake, no
other dependencies. Written from `../SPEC.md`, using the frozen art in `../assets/`.

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
| `E` | Open the magazijn door |
| `Esc` | Quit |
| `R` | Restart, on the death or victory screen |

Find the bedrijfsleider's pass in the freezer, open the magazijn, touch the loading
dock door. Twenty-four things in the shop would rather you did not.

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
- **Three enemies that want different things.** The winkelwagen charges the moment it
  sees you and does not stop. The vakkenvuller holds about seven metres, sidesteps, and
  lobs soup cans in an arc you can walk out of — real projectiles, stopped by shelves.
  The zelfscanner barely moves, paints you with a targeting line for half a second, and
  then burst-fires down the aisle: standing in the open is a decision, not an accident.
- **The level.** Hand-authored as ASCII in `src/map.cpp` — checkouts, six gondola
  aisles with sightlines down each, a freezer, and the back of house behind a locked
  door. The keycard is in the far corner of the freezer, and there is a turret already
  looking at the door you have to come through.
- **Sound.** Synthesised in code at startup: the asset pack ships no audio and the spec
  forbids fetching any. A dozen oscillator-and-noise sounds, plus a refrigeration hum.
  Pickups play a barcode-scanner beep, because they had to.

## Code

Ten small modules, no framework:

| File | What it owns |
|---|---|
| `map.*` | Tiles, the hard-coded level, doors, line of sight, circle-vs-wall sliding |
| `player.*` | Input, movement, both weapons, the keycard door |
| `enemy.*` | The three behaviours, and the soup can |
| `pickup.*` | The five pickups |
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
