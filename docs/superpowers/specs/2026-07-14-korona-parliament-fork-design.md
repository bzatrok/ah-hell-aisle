# Design — „Országház: Pokoli Ülésszak" (korona fork)

**Date:** 2026-07-14 · **Status:** approved by Ben · **Implements as:** handover 009, branch `korona`

A full-theme fork of the bake-off winner (`claude/`): the Albert Heijn becomes the
Hungarian Parliament, the arsenal becomes the coronation regalia, and the manager-boss
becomes a fictional political archetype at the rostrum of the Ülésterem. Gameplay
systems are inherited from handover 007's end state; the only genuinely new mechanic
is the Szent Korona's returning throw.

## 1. Ground rules (non-negotiable)

- **Fictional cast only.** Enemies are satirical political-machine *archetypes*. No
  real person's name, likeness, or thinly-veiled variant thereof anywhere in code,
  assets, strings, or docs. This constraint carries into the handover verbatim.
- No real-world anthems, party logos, or official emblems in art or audio. The
  Szent Korona / regalia as *objects* are fine (museum pieces, public heritage).
- The fork honours the repo rules that survive onto a branch: `runner-ups/` is
  untouched, and all art changes go through `tools/gen_assets.py` so `assets/`
  stays regenerable from one script.

## 2. Fork mechanics

- **Branch `korona` off `main`, cut only after handover 007's row is ✅** in
  `.handovers/handover_log.md`. The fork is written against 007's end state
  (4 weapon slots, weapon/ammo pickups, 5 enemy kinds incl. the bedrijfsleider
  boss, 3 levels, SFX + music). Same gating precedent as handover 008.
- Handover 008 (mobile controls) is independent: if it lands on `main` before the
  branch is cut, the fork inherits it for free; if after, merging `main` into
  `korona` later is optional polish, not a prerequisite.
- Everything re-themes **in place** on the branch: `claude/src`, `tools/gen_assets.py`,
  `assets/`. Unlike 007 (which could only *add* generator functions), the branch
  rewrites existing generator functions freely — that is the point of the fork.
- Game title: **„Országház: Pokoli Ülésszak"**. Binary: `pokoli_ulesszak`
  (CMake target rename). Window title, HUD strings, and on-screen text go Hungarian.
- Known branch quirk (accepted): regenerating `assets/` means the archived
  runner-ups' `../../assets/` fallback shows parliament art *on this branch*.
  `main` is unaffected.
- **Web deploy is out of scope.** `./web/build.sh` must keep working on the branch,
  but deploying would overwrite the `ah-hell-aisle` Cloudflare Pages project. A
  separate Pages project is a later, separate decision.

## 3. Setting — three acts of the Országház

The three levels from 007 are re-authored (new ASCII grids in `map.cpp`, same
40×40 format, same tile-kind count) as a route through the building:

| Level | Act | Beats |
|---|---|---|
| 1 | **Főlépcsőház + folyosók** — grand staircase, red-carpet corridors, offices | Spawn with jogar only; kard pickup mid-level, defended (takes 007's `g` slot) |
| 2 | **Kupolacsarnok wing** | The Szent Korona is picked up where the real one is displayed — under the dome. Országalma pickup behind the badge-locked door (007's `p` slot). |
| 3 | **Ülésterem finale** | The semicircular assembly hall; boss at the rostrum; exit gate opens on boss death. |

Tile re-theme (same 9 `Tile` kinds, new art):
shelves → bench rows / gothic wood panelling · freezer → marble colonnade + stained
glass · checkout → rostrum front · magazijn → archive stacks · plain → gothic stone ·
keycard door → ornate door with badge reader · exit → main gate · floor → red carpet
on parquet · ceiling → gothic vaulting. The 4 lighting `Zone`s become: csarnok (warm
gold), folyosó (red-warm), kripta/alagsor (the flicker zone — failing chandeliers),
ülésterem (dramatic). Palette: AH blue out; gold, burgundy, marble, dark wood in,
tricolor accents on the HUD only.

## 4. Weapons — the four-piece regalia

| Slot | Was | Becomes | Mechanic | Ammo | Code impact |
|---|---|---|---|---|---|
| 1 | stokbrood | **Jogar** (scepter) | melee swing | free | reskin |
| 2 | prijspistool | **Szent Korona** | thrown; flies flat, damages on hit, then returns to the player; cannot throw again until caught | free — the return trip is the rate limiter | **the one new mechanic** |
| 3 | statiegeldkanon | **Szent István kardja** (sword) | the pellet spread reads as one wide sweeping slash | free; cooldown ~2× to compensate | reskin + rebalance |
| 4 | vuurwerkpijl | **Országalma** (orb) | lobbed, explodes with radial falloff, hurts you too | **szenteltvíz** vials (reskinned vuurwerk pickups) | reskin; optional: give it the soup-can arc instead of flat flight |

Korona implementation sketch: new `Projectile::Kind::Korona` with `ownerIsPlayer`,
outbound leg at fixed speed; on enemy hit apply damage and flip to the return leg;
on wall hit flip to return; return leg homes on the player's live position; contact
re-arms slot 2. While out, slot 2 shows a "crown out" HUD state and cannot fire.
`PlayerAmmoPool` returns null for slots 1–3. Start ownership changes from 007's
`{true, true, false, false}` to `{true, false, false, false}` — only the jogar at
spawn; korona, kard and országalma are map pickups (a third weapon-pickup kind,
following the WeaponScatter/WeaponRocket pattern).

Ammo economy collapses to **one pool** (szenteltvíz for the országalma). The label
and flessen pools, their pickups, and their map-legend spawns are removed; freed
spawn tiles become health/armour. This simplifies `hud.cpp`'s ammo panel.

## 5. Enemies — five reskins, zero new AI

007's end-state roster maps 1:1; behaviours are inherited, only art/names/strings
change:

| 007 enemy (AI) | Becomes | Flavour |
|---|---|---|
| winkelwagen (charger) | **Szavazógép** | a rogue voting terminal on castors that rams you |
| vakkenvuller (thrower) | **Propagandista** | hurls **nemzeti konzultáció** envelopes (soup-can projectile reskin) |
| zelfscanner (frozen-aim beam turret) | **Kamera-drón** | state-TV camera drone; telegraph = red REC light, beam = blinding spotlight |
| beveiliger (guard, sidearm) | **Testőr** | black suit, earpiece |
| bedrijfsleider (boss) | **„Az Örök Elnök"** | giant suited figure at the rostrum; a fictional archetype by design |

## 6. Pickups & HUD

| Was | Becomes | Effect (unchanged) |
|---|---|---|
| appelflap | **pogácsa** | +25 HP |
| rookworst | **gulyás** | +50 HP |
| bonuskaart | **mentelmi jog** card | +50 armour — parliamentary immunity as armour |
| keycard | **képviselői belépő** | opens the badge door |
| vuurwerk | **szenteltvíz** | +4 országalma charges |
| labels, flessen | *removed* | pools deleted (see §4) |
| weapon pickups | kard, korona, országalma | as placed in §3 |

HUD: gothic wood/gold panel, crowned face (all existing damage states regenerated),
labels in Hungarian — Élet / Mentelmi / Áldás (ammo). Sounds: 007's SFX set renamed
and re-pitched where the flavour demands (crown whoosh/catch, sword sweep, orb blast,
envelope flutter); the procedural music stays procedural — solemn, organ-adjacent
retuning is the executor's call. No real melodies.

## 7. Assets pipeline

`tools/gen_assets.py` is re-themed on the branch: same deterministic seed-in →
PNGs-out contract, same file-per-slot naming (new names where things were renamed),
same sheet layouts (6-frame enemies, 3-frame weapons, boss at 96px cell).
`assets/MANIFEST.md` rewritten to match. After a clean checkout + one script run,
`git status` must be clean — regenerability is the acceptance test.

## 8. Verification

- Native build from `claude/` and a full three-level playthrough: spawn → kard →
  korona under the dome → badge door → országalma → boss → exit.
- Korona-specific: throw into a wall (returns), throw and sidestep (still finds
  you), attempt double-throw (refused), catch re-arms.
- `./web/build.sh` still produces a running build (no deploy).
- `python3 tools/gen_assets.py` twice → identical bytes.

## 9. Staging (order for the executing agent)

1. Branch cut + rename pass (title, binary, strings) — game still fully playable AH-themed.
2. Asset generator re-theme + regenerate (walls, floor, ceiling, HUD, pickups, enemies, weapons) — playable at every step.
3. Weapon mechanics: kard rebalance → országalma reskin/arc → korona returning throw.
4. Ammo-economy collapse + HUD simplification.
5. Three new level grids + zone lighting.
6. Boss reskin + finale wiring; SFX/music pass.
7. Full verification (§8).
