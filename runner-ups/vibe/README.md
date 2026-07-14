# AH: Hell Aisle - Doom 2 Clone

A complete Doom 2 clone set in an Albert Heijn supermarket after closing time, built with C++17 and raylib.

## Build Instructions

From the `vibe/` directory:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

## Requirements

- C++17 compiler
- raylib (install with: `brew install raylib`)
- CMake (install with: `brew install cmake`)

## Controls

- **WASD**: Move
- **Mouse**: Look (X-axis only)
- **Left Mouse / Ctrl**: Fire
- **1**: Select Stokbrood (melee)
- **2**: Select Prijspistool (ranged)
- **E**: Use/open doors
- **R**: Restart (on death/victory screens)
- **Esc**: Quit

## Game Features

- 48x48 tile-based supermarket map with distinct zones:
  - Entrance/Checkout area
  - Aisles with shelf gondolas
  - Freezer section (with darker lighting)
  - Magazijn (back room)
- Three enemy types (20+ total):
  - Winkelwagen: Fast melee charger (30 HP, 10 dmg)
  - Vakkenvuller: Ranged, throws soup cans (60 HP, 15 dmg)
  - Zelfscanner: Anchored turret with hitscan (100 HP, 8 dmg)
- Two weapons:
  - Stokbrood: Melee bread bat (25 damage, 2 swings/sec)
  - Prijspistool: Ranged label gun (20 damage, 4 shots/sec, uses label ammo)
- Pickups:
  - Appelflap: +25 health
  - Rookworst: +50 health
  - Labels: +20 ammo (max 200)
  - Bonuskaart: +50 armor (max 100, absorbs 50% damage)
  - Keycard: Opens magazijn door
- Full HUD with health, ammo, armor indicators
- Death screen (GESLOTEN) and victory screen with stats
- Proper collision detection (circle vs tile)
- Line-of-sight for enemies (raycasting)
- Keycard-locked door to magazijn
- Exit door leads to victory

## Implementation Notes

- Single-file implementation in src/main.cpp
- Uses raylib's Vector2, Vector3, Camera3D types
- Entity-based architecture with polymorphism
- Simple raycasting for line-of-sight and hitscan
- Billboard sprites for enemies, pickups, and projectiles
- Textured walls, floor, and ceiling using raylib models and materials
- All 24 required assets loaded and used (including floor.png and ceiling.png)

## What Was Cut

The following were simplified or not implemented to meet the spec's restraint criterion:
- No advanced pathfinding (simple "move toward player, slide on walls")
- No sound effects
- No door opening animations
- No weapon muzzle flash
- No particle effects

## Known Issues Fixed

- Fixed exit door trigger (was using `dx <= 0 && dy <= 0` which was always false)
- Doors now properly block movement when closed (player and enemies)
- Hitscan (Prijspistool) now properly blocked by walls and doors
- Zelfscanner hitscan now properly blocked by doors
- Enemy line-of-sight properly blocked by doors
- Projectiles (soup cans) now properly blocked by walls and doors
- Kill counting works for both weapons
- Player collision with doors fixed
- Textured walls, floor, and ceiling implemented
- Freezer section has darker lighting
- Sprite depth sorting implemented for proper occlusion
- Added small random spread to Prijspistool
- Enemies now respect closed doors and cannot walk through them

All core gameplay requirements from SPEC.md sections 4-10 are implemented.
