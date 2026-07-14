# Handover 009 — The korona fork: „Országház: Pokoli Ülésszak"

**Branch:** `korona` (create from `main` — see the gate in §0)  ·  **Author:** brainstorm session with Ben, 2026-07-14  ·  **Status:** pending
**Scope:** a full-theme fork of the winning game (`claude/`): the Albert Heijn becomes the Hungarian Parliament, the four weapons become the coronation regalia, the five enemies become satirical political-machine archetypes, and the three levels become a route through the Országház ending with a boss in the Ülésterem. Gameplay systems are inherited from handover 007's end state; the only genuinely new mechanic is the Szent Korona's returning throw. Design rationale: `docs/superpowers/specs/2026-07-14-korona-parliament-fork-design.md` (committed; every operational decision is also inlined below).

## 0. Shared context (read first)

**Prerequisite gate — do not start unless ALL of these hold:**
1. `.handovers/handover_log.md` row **007** is ✅ done.
2. You are on `main` and the expansion is merged: `grep -q Bedrijfsleider claude/src/enemy.h` succeeds and `grep -q "kLevelCount = 3" claude/src/map.h` succeeds.
3. Handover **008 (mobile controls) is explicitly irrelevant** — do not wait for it. If it landed on `main` before you branch, you inherit it; fine either way.

If the gate holds: `git checkout -b korona main` and do all work on that branch. Never merge `korona` into `main` — it is a permanent fork branch.

**Hard content constraint (non-negotiable, from the author session):** the enemy cast is *fictional satirical archetypes only*. No real person's name, likeness, or thinly-veiled variant anywhere in code, assets, strings, or docs. No real-world anthems, party logos, or official emblems. The regalia (Szent Korona, jogar, országalma, Szent István kardja) as museum objects are fine. If you are ever tempted to make something "more recognizable" as a real individual — don't.

**Repo rules** (`CLAUDE.md` at repo root): never touch `runner-ups/`; all art goes through `tools/gen_assets.py` so `assets/` stays regenerable. On this branch you may **modify existing generator functions freely** (007 could only add; the fork re-themes). Known accepted quirk: regenerating `assets/` means the runner-ups' `../../assets/` fallback shows parliament art on this branch; `main` is untouched.

**Build** (from repo root, per `CLAUDE.md`):

```sh
# native
cmake -B claude/build -S claude -DCMAKE_BUILD_TYPE=Release && cmake --build claude/build
./claude/build/<binary>          # ah_hell_aisle before task A, pokoli_ulesszak after
# assets
python3 tools/gen_assets.py
# web (build must keep working; DO NOT deploy — see Decisions)
./web/build.sh
```

**Key files** (007 is reorganizing as this is written — anchors are symbols, not line numbers; grep for them):

| Concern | Where |
|---|---|
| Weapon enum + player state | `claude/src/player.h` — `WeaponId`, `hasWeapon`, `PlayerAmmoPool` |
| Fire functions, weapon select | `claude/src/player.cpp` — `FireStokbrood/FirePrijspistool/FireStatiegeldkanon/FireVuurwerkpijl`, `SelectWeapon` |
| Tuning constants | `claude/src/config.h` (weapon damage/cooldowns/rocket splash) |
| Enemy kinds, stats, AI | `claude/src/enemy.h` (`EnemyKind`), `claude/src/enemy.cpp` (`kStats`, per-kind update fns) |
| Projectiles (soup can arc, rocket splash) | `claude/src/enemy.h` — `Projectile` (`kind`, `ownerIsPlayer`, `height`/`vy` arc), `enemy.cpp` — `ProjectilesUpdate` |
| Pickups | `claude/src/pickup.h` (`PickupKind`), `pickup.cpp`, spawn legend in `claude/src/world.cpp` |
| Levels (ASCII grids) | `claude/src/map.cpp` — `kLevel*`, `TileFor`, `ZoneFor`, `LoadLevel`; `map.h` — `Tile`, `Zone`, `kLevelCount` |
| Rendering (billboards, weapon draw, boss 96px cell) | `claude/src/render.cpp` |
| HUD (health/armour/ammo panel, face) | `claude/src/hud.cpp` |
| SFX/music | `claude/src/audio.h/cpp` |
| Asset generator + contract | `tools/gen_assets.py`, `assets/MANIFEST.md` |
| Binary/target name | `claude/CMakeLists.txt`; web harness references in `web/build.sh` |

## A — Rename pass  *(DECIDED — do first, game stays playable throughout)*

Title: **„Országház: Pokoli Ülésszak"**. CMake target/binary: `pokoli_ulesszak` (update `claude/CMakeLists.txt` and whatever `web/build.sh` greps for the binary/target name). Window title, title screen, HUD strings, on-screen messages (locked-door nag, death line, win line) go Hungarian. Executor writes the actual lines — keep them short, dry, and diacritic-correct (the font path must render á/é/í/ó/ö/ő/ú/ü/ű; if the current text rendering is ASCII-only raylib default font, either load a font with Hungarian glyphs or spell without diacritics consistently — decide by testing, both acceptable).

**Files:** `CMakeLists.txt` · `main.cpp` · `hud.cpp` · `web/build.sh` (+ `web/` html title if present)

## B — Asset generator re-theme  *(DECIDED — regenerate after each function so the game is playable at every step)*

Rewrite drawing functions in `tools/gen_assets.py` in place; same deterministic seed-in → PNGs-out contract, same sheet layouts (6-frame enemies: idle, walk, attack, die×3 · 3-frame weapons · boss at 96px cell). Rename output files where the thing itself is renamed and update every `Grab("…")` in `claude/src/assets.cpp` plus `assets/MANIFEST.md`. Palette: AH blue out; gold, burgundy, marble, dark wood; tricolor accents on the HUD only.

| Asset (was) | Becomes |
|---|---|
| wall_shelf_full / wall_shelf_empty | bench rows / gothic wood panelling |
| wall_freezer | marble colonnade + stained-glass window |
| wall_checkout | rostrum front |
| wall_magazijn | archive stacks |
| wall_plain | gothic stone |
| door_keycard | ornate door with badge reader |
| door_exit | main gate |
| floor / ceiling | red carpet on parquet / gothic vaulting |
| enemy_winkelwagen | **enemy_szavazogep** — rogue voting terminal on castors |
| enemy_vakkenvuller | **enemy_propagandista** — hurls konzultáció envelopes |
| enemy_zelfscanner | **enemy_kameradron** — state-TV camera drone; REC-light telegraph, spotlight beam |
| enemy_beveiliger | **enemy_testor** — black suit, earpiece |
| enemy_bedrijfsleider | **enemy_orok_elnok** — „Az Örök Elnök", giant suited figure (96px cell stays) |
| proj_soepblik | **proj_konzultacio** — flying envelope |
| proj_vuurwerkpijl | **proj_orszagalma** — the orb, cross on top |
| weapon_stokbrood | **weapon_jogar** (scepter) |
| weapon_prijspistool | **weapon_korona** (the Szent Korona in your hands; bent cross) |
| weapon_statiegeldkanon | **weapon_kard** (Szent István kardja) |
| weapon_vuurwerkpijl | **weapon_alma** (országalma held ready to lob) |
| pickup_appelflap | **pickup_pogacsa** (+25 HP) |
| pickup_rookworst | **pickup_gulyas** (+50 HP) |
| pickup_bonuskaart | **pickup_mentelmi** (+50 armour — an immunity card) |
| pickup_keycard | **pickup_belepo** (képviselői belépő badge) |
| pickup_vuurwerk | **pickup_szenteltviz** (+4 országalma charges) |
| pickup_labels, pickup_flessen | **deleted** (see §F) |
| pickup_statiegeldkanon / pickup_vuurwerkpijl | **pickup_kard / pickup_alma** weapon pickups, **plus new pickup_korona** |
| hud_panel / hud_face | gothic wood/gold panel / crowned face, all damage states |

**Files:** `tools/gen_assets.py` · `assets/MANIFEST.md` · `claude/src/assets.h/cpp`

## C — Kard: scatter → sweeping slash  *(DECIDED)*

`FireStatiegeldkanon` becomes `FireKard`: keep the multi-pellet hitscan spread (it reads as one wide sweep), remove the ammo cost, and double the cooldown constant to compensate for free ammo. Rename enum values (`WeaponId::Statiegeldkanon` → `Kard`, etc. across all four — do the enum rename once, here). SFX: a sweep/whoosh replaces the scatter bang (`audio.cpp` synth tweak).

**Files:** `player.h/cpp` · `config.h` · `audio.h/cpp`

## D — Országalma: rocket reskin + lob arc  *(DECIDED, arc optional)*

`FireVuurwerkpijl` becomes `FireAlma`; splash behaviour (radial falloff, hurts the player too) is inherited unchanged. Optional polish if it costs little: launch with the soup can's arc (`Projectile.height`/`vy`) instead of flat flight — if the splash-on-impact code assumes flat height, skip the arc rather than fight it. Ammo pool renames to szenteltvíz (see §F).

**Files:** `player.cpp` · `config.h` · `enemy.cpp` (projectile) · `render.cpp`

## E — Szent Korona: the returning throw  *(DECIDED — the one new mechanic)*

Replaces the prijspistool in slot 2. No ammo; the return trip is the rate limiter.

- `Projectile::Kind` gains `Korona`; thrown with `ownerIsPlayer = true`, flat flight at a speed comparable to the rocket, new `kKorona*` constants in `config.h` (damage ~kGunDamage×2 per hit; executor tunes).
- Outbound leg: on enemy hit → `EnemyHurt` and flip to return leg (do not stop); on wall hit → flip to return leg; pierce-through on the way back is allowed and encouraged (it makes the throw-through-a-crowd play).
- Return leg: home on the player's **live** position each frame; on contact, re-arm slot 2. Track "crown is out" as player state (e.g. `bool koronaOut` on `Player`) — while out, slot 2 cannot fire and the HUD weapon panel shows an empty-hands frame (add a 4th frame to `weapon_korona` sheet or reuse frame logic; executor's call).
- Player death while the crown is out needs no special case (projectile dies with the world reset).
- Start ownership changes from `{true, true, false, false}` to `{true, false, false, false}` — only the jogar at spawn. Korona, kard, alma are map pickups; add the **korona weapon pickup** as a new `PickupKind` following the existing `WeaponScatter`/`WeaponRocket` pattern. `PlayerAmmoPool` returns null for slots 1–3.
- SFX: throw whoosh, metallic catch, enemy-hit ring (`audio.cpp`).

**Files:** `player.h/cpp` · `enemy.h/cpp` (projectile) · `pickup.h/cpp` · `world.cpp` (spawn legend char for the korona pickup) · `config.h` · `render.cpp` · `hud.cpp` · `audio.h/cpp`

## F — Ammo economy collapses to one pool  *(DECIDED)*

Only the országalma consumes ammo (szenteltvíz). Delete the label and flessen pools (`Player::ammo`, `Player::flessen`), their `PickupKind`s, spawn-legend chars, and map placements; freed spawn tiles become pogácsa/mentelmi. Rename `Player::vuurwerk` → `szenteltviz` (`kMaxVuurwerk` → `kMaxSzenteltviz`). `hud.cpp`: the ammo panel shows one number, labelled **Áldás**; health = **Élet**, armour = **Mentelmi**.

**Files:** `player.h/cpp` · `pickup.h/cpp` · `world.cpp` · `map.cpp` (spawn chars) · `hud.cpp` · `config.h`

## G — Three new level grids: the Országház route  *(DECIDED)*

Re-author the three ASCII grids in `map.cpp` (same 40×40 format, same 9 `Tile` kinds, same spawn-legend mechanism). Rising difficulty as in 007.

| Level | Act | Beats |
|---|---|---|
| 1 | Főlépcsőház + red-carpet corridors, offices | spawn with jogar only; **kard pickup mid-level, defended** |
| 2 | Kupolacsarnok wing | **korona pickup under the dome** (staged like the real display: centre of the dome hall, guarded); **alma pickup behind the belépő-locked door**; the belépő badge is this level's key |
| 3 | Ülésterem finale | semicircular assembly hall (benches as `Tile` walls in concentric arcs facing the rostrum); **Az Örök Elnök at the rostrum**; exit gate opens on boss death |

The 4 lighting `Zone`s re-map to: csarnok (warm gold) · folyosó (red-warm) · alagsor (the flicker zone — failing chandeliers; use it for at least one creepy basement stretch) · ülésterem (dramatic). Keep zone *behaviour* (ambient colour + flicker) untouched; only colours and placement change.

**Files:** `map.cpp` · `map.h` (zone names if renamed) · `render.cpp` (zone ambients)

## H — Boss + finale + audio pass  *(DECIDED)*

The bedrijfsleider's AI is inherited untouched as **Az Örök Elnök** — rename `EnemyKind`, swap the 96px sheet (§B), re-voice his SFX (`AlertBoss` etc.) to something gavel/echo flavoured. Music: 007's procedural tracks stay procedural; retune toward solemn/organ-adjacent if the synth makes it cheap, otherwise leave. **No real melodies** (no anthem quotes).

**Files:** `enemy.h/cpp` · `audio.h/cpp` · `assets.cpp`

## Sequencing

1. Gate check (§0) → branch `korona`.
2. **A** rename pass → build + run: AH game, Hungarian chrome.
3. **B** asset re-theme, regenerating and eyeballing per function → build + run after each family (walls → HUD → pickups → enemies → weapons).
4. **C** kard → **D** alma → **E** korona (mechanics, hardest last) → build + run after each.
5. **F** ammo collapse (touches everything C–E renamed — do after them).
6. **G** levels → **H** boss/audio.
7. Acceptance checklist below; commit small and often throughout (per CLAUDE.md style).

## Decisions — locked ✅

- Fictional archetypes only; the content constraint in §0 is a hard author-session decision, not style guidance.
- Fork lives on branch `korona` forever; never merges to `main`. Gated on 007 ✅; 008 irrelevant.
- Title „Országház: Pokoli Ülésszak", binary `pokoli_ulesszak`.
- Four regalia weapons in the existing four slots: jogar (melee, free) · Szent Korona (returning throw, free, the only new mechanic) · kard (sweep = scatter reskin, free, ~2× cooldown) · országalma (splash lob, szenteltvíz ammo).
- Only jogar owned at spawn; korona/kard/alma are map pickups (kard L1, korona L2 dome, alma L2 behind the badge door).
- One ammo pool (szenteltvíz); label + flessen pools and pickups deleted.
- Five enemy reskins, zero new AI: szavazógép ← winkelwagen · propagandista ← vakkenvuller · kamera-drón ← zelfscanner · testőr ← beveiliger · Az Örök Elnök ← bedrijfsleider.
- Pickups: pogácsa +25 · gulyás +50 · mentelmi jog +50 armour · képviselői belépő key · szenteltvíz +4.
- Three levels re-authored as Főlépcsőház → Kupolacsarnok → Ülésterem.
- Web build must keep working; **no deploy** (would overwrite the `ah-hell-aisle` Pages project — a separate Pages project is a future decision, not yours).
- `assets/` divergence on this branch (runner-ups fallback shows parliament art) is accepted.

## Acceptance / pre-merge checklist

- [ ] `python3 tools/gen_assets.py` run twice from a clean tree → `git status` clean the second time (byte-identical output).
- [ ] Native build clean; binary is `pokoli_ulesszak`.
- [ ] Full three-level playthrough: spawn (jogar only) → kard mid-L1 → korona under the dome → belépő → alma → Ülésterem boss → exit gate. Win screen reached.
- [ ] Korona mechanics: throw into a wall (returns) · throw then sidestep (still catches you) · double-throw attempt refused while out · catch re-arms · pierces on return leg.
- [ ] Kard consumes no ammo; alma consumes szenteltvíz and its blast hurts you at point blank.
- [ ] HUD shows Élet / Mentelmi / Áldás; crowned face damage states cycle.
- [ ] No real person's name/likeness anywhere: `grep -riE "orban|orbán" claude/ tools/ assets/` (and any other real names you were tempted by) returns nothing.
- [ ] `./web/build.sh` completes and the web build runs locally (title screen + level 1 visible). **Not deployed.**
- [ ] `runner-ups/` untouched: `git diff main..korona --stat -- runner-ups/` is empty.

---
**On completion:** update `.handovers/handover_log.md` — set row 009 to ✅ done and fill the
Completed date. If you could not finish, set 🔄 in-progress and append a note describing what
remains.
