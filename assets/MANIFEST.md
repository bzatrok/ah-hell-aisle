# Shared asset manifest

Every file here is **frozen and identical for all four competitors**. Do not edit,
regenerate, or add to this folder. Load it read-only from `../assets/`.

All files are 32-bit RGBA PNG. Transparency is a real alpha channel — there is no
magenta colour key. Draw sprites with alpha blending; do not premultiply.

Regenerated deterministically by `tools/gen_assets.py` (seed 20260713) if ever lost.

## Walls, floor, ceiling — 64x64, opaque, tileable

| File | Use |
|---|---|
| `wall_shelf_full.png` | Stocked gondola shelving. The default aisle wall. |
| `wall_shelf_empty.png` | Picked-clean shelving. Use to signal looted areas. |
| `wall_freezer.png` | Glass freezer doors. The cold aisle. |
| `wall_plain.png` | Painted wall with the AH blue band. Store perimeter. |
| `wall_checkout.png` | Checkout lane: belt, register, logo. |
| `wall_magazijn.png` | Corrugated steel. Back-of-house. |
| `door_keycard.png` | Locked stockroom door. Needs the keycard. |
| `door_exit.png` | Loading-dock door. Touching it ends the level. |
| `floor.png` | Tiled shop floor. |
| `ceiling.png` | Ceiling panel with a strip light. |

## Enemies — horizontal 6-frame strips, 64x64 per frame (sheet = 384x64)

Frame order is identical for all three, index left to right:

| Index | Frame |
|---|---|
| 0 | walk A |
| 1 | walk B |
| 2 | attack |
| 3 | death A |
| 4 | death B |
| 5 | death C (final corpse — hold this frame) |

| File | Enemy |
|---|---|
| `enemy_winkelwagen.png` | Rogue trolley. Fast melee charger. |
| `enemy_vakkenvuller.png` | Undead shelf-stocker. Lobs soup cans. |
| `enemy_zelfscanner.png` | Self-checkout terminal. Hitscan turret. |

These are single-angle sprites — they always face the camera. There is no
8-direction rotation sheet, so you do not need `angle` logic for enemies.

## Projectile

| File | Size | Use |
|---|---|---|
| `proj_soepblik.png` | 16x16 | The Vakkenvuller's thrown soup can. Billboard it. |

## Pickups — 32x32, billboarded

| File | Effect |
|---|---|
| `pickup_appelflap.png` | +25 health |
| `pickup_rookworst.png` | +50 health |
| `pickup_labels.png` | +20 label ammo |
| `pickup_bonuskaart.png` | +50 armour |
| `pickup_keycard.png` | The bedrijfsleider's pass. Opens `door_keycard`. |

## Weapons — horizontal 3-frame strips, 192x144 per frame (sheet = 576x144)

| Index | Frame |
|---|---|
| 0 | idle |
| 1 | fire / swing — windup |
| 2 | fire / swing — impact |

| File | Weapon |
|---|---|
| `weapon_stokbrood.png` | Baguette. Melee, infinite. |
| `weapon_prijspistool.png` | Price-label gun. Hitscan, uses label ammo. |

The art is drawn anchored to the **bottom-centre-right** of its frame — the hand
enters from below the screen edge. Draw it bottom-aligned and horizontally
centred (or slightly right of centre), scaled up to taste. Bob it while walking.

## HUD

| File | Size | Notes |
|---|---|---|
| `hud_face.png` | 144x56 | 3-frame strip, 48x56 each: `0`=ok, `1`=hurt (<50% hp), `2`=dead. |
| `hud_panel.png` | 320x48 | Status-bar background. Stretch across the screen bottom; it has four recessed cells for HEALTH / AMMO / ARMOUR / KEY. |

Numbers and text on the HUD: use raylib's built-in font (`DrawText`). No bitmap
font is shipped.

## Audio

None shipped. Sound is **optional and unscored**. If you want it, synthesise it
in code with raylib's audio API. Do not download audio files.
