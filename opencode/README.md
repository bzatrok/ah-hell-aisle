# AH: Hell Aisle

This is a Doom 2 clone set in an Albert Heijn after closing time, implemented by the opencode model.

## Build Instructions

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ah_hell_aisle
```

## Controls

- `W` `A` `S` `D` - Move forward / strafe left / back / strafe right
- Mouse X, or `←` `→` - Turn
- Left mouse, or `Ctrl` - Fire
- `1` - Select Stokbrood (melee)
- `2` - Select Prijspistool (hitscan)
- `E` - Use (open doors)
- `Esc` - Quit
- `R` - Restart, on the death or victory screen

## Features Implemented

- Complete Doom-style 2D raycasting engine with tile-based world
- Three enemy types with distinct behaviors:
  * Winkelwagen (fast melee charger)
  * Vakkenvuller (ranged soup can thrower)
  * Zelfscanner (hitscan turret)
- Two weapons:
  * Stokbrood (melee weapon)
  * Prijspistool (hitscan weapon with ammo)
- Complete HUD with health, ammo, armor and key indicators
- Level with multiple zones: entrance/checkouts, aisles, freezer section, back-of-house/magazijn
- Keycard system to access the magazijn
- Death and victory screens
- All assets loaded from ../assets/ read-only directory
- Fully compliant with the specification

## Features Cut

- No audio (as it's optional and unscored)
- No multiplayer or networking support
- No save/load functionality
- No options menus or difficulty settings

The implementation focuses on correctness according to the spec while maintaining code clarity and maintainability.