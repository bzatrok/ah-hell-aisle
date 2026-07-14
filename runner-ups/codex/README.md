# AH: Hell Aisle

A compact Doom-style raycaster set in an Albert Heijn after closing. You are a
night-shift vakkenvuller: collect the bedrijfsleider's keycard, enter the
magazijn, and escape through the loading dock.

## Build

From this directory (with Homebrew raylib installed):

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

## Controls

- `W` `A` `S` `D`: move / strafe
- Mouse X or Left/Right arrows: turn
- Left mouse or `Ctrl`: fire
- `1`: Stokbrood (melee)
- `2`: Prijspistool (hitscan; uses labels)
- `E`: use the keycard door
- `Esc`: quit
- `R`: restart after death or victory

## Included

One 32×32 hand-authored store, textured walls/floor/ceiling, depth-occluded
billboard sprites, all three enemy types, soup-can projectiles, both weapons,
all pickups, keycard/door progression, HUD, title, death, and victory screens.

## Cuts

There is no audio, sliding-door animation, or enemy pathfinding. Audio is
optional; doors open instantly; enemies use line-of-sight and simple collision
sliding, which keeps their behaviour readable in the aisle maze.
