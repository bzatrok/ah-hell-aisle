# Handover 007 — Kassa fix, web Esc, +2 weapons, +2 enemies, 3 levels, SFX + music

**Branch:** tasks A–C directly on `main` (small fixes); tasks D–J on a new `feature/expansion` branch, merged to `main` when the acceptance checklist passes.
**Author:** session 2026-07-14 (the session that built the web harness, handover 006)  ·  **Status:** pending
**Scope:** the bake-off is decided — `claude/` won and is unlocked (commit `2434451`). This handover turns the winning entry into a bigger game: fix the undodgeable self-checkout turret, fix Esc on the web build, add two weapons, two enemies, two more levels with rising difficulty, and give everything a voice (SFX + procedural music). Builds on handover 006 (`web/` WASM harness, commits `6b20f7c`, `1e11da5`).

## 0. Shared context (read first)

Repo root is `/…/random_stuff/`. Read `CLAUDE.md` (repo root) for the post-competition rules; the two that matter here: **never touch `runner-ups/`**, and **new art goes through `tools/gen_assets.py`** so `assets/` stays regenerable. `SPEC.md` describes the game as it stands — its §11 "non-goals" (no multiple levels etc.) applied to the *competition* and no longer bind.

The game is compact (~2.4k lines, C++17 + raylib 6.0) and rigorously data-driven; every addition below follows an existing pattern. Positions are `Vector2` where `.y` is world Z; one tile = one world unit.

| Concern | Where |
|---|---|
| All tuning constants | `claude/src/config.h` |
| ASCII level + spawn legend | `claude/src/map.cpp:21-62` (`kLevel`), `:64` `TileFor`, `:79` `ZoneFor`, `:86` `LoadLevel` |
| Map struct, doors, LOS/DDA, collision | `claude/src/map.h` |
| Entity spawn switch | `claude/src/world.cpp:11-45` (`WorldInit`) |
| Enemy stats table | `claude/src/enemy.cpp:13-17` (`kStats`), header `enemy.h:16-27` |
| Per-enemy AI | `enemy.cpp:95` cart, `:133` stocker, `:200` zelfscanner (`kWindup` at `:202`, burst fire `:217-241`) |
| Projectiles (soup can) | `enemy.h:50`, `enemy.cpp:305` (`ProjectilesUpdate`) |
| Weapons | `player.h:9` (`WeaponId`), `player.cpp:74` melee, `:99` hitscan, `:136` select/fire, `:230` `PlayerDamage` |
| Synth audio, `Render(seconds, fn)` helper | `claude/src/audio.cpp` (`Render` at `:42`, hum loop pattern at `:173-203`), enum `audio.h:5` |
| Texture registry | `claude/src/assets.h:7` (arrays sized by kind counts), `assets.cpp:26` (path probe + `Grab` list) |
| Renderer: baked meshes, billboards, beams, screens | `claude/src/render.cpp` — shaders at `:32/:50/:78` (`#version 330`), grep `aimBeam` for the turret telegraph |
| Game states | `claude/src/game.cpp`, `game.h` (`Title/Playing/Dead/Escaped`) |
| One-time geometry bake | `claude/src/main.cpp:19` (`RenderInit` — called once, comment says level never changes: task H changes this) |
| Web build | `web/build.sh` (shader sed at `:38`, link flags `:48-58` — **keep `-sGROWABLE_ARRAYBUFFERS=0`**, it fixes a Chrome WebGL bug), shell page `web/index.html` |
| Art generator + manifest | `tools/gen_assets.py`, `assets/MANIFEST.md` — read both before task E |

Build & run:

```sh
# native (from claude/)
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/ah_hell_aisle
# web (from repo root; brew install emscripten if em++ missing)
./web/build.sh && python3 -m http.server 8080 -d web/dist
```

Spawn-legend characters already taken: `# C S s F M K X` (tiles), `@ 1 2 3` (player/enemies), `a r l b k` (pickups). This handover adds `4 5` (enemies), `g p` (weapon pickups), `f v` (ammo pickups).

---

## A — Zelfscanner ("kassa" turret) is undodgeable and overtuned  *(BLOCKER, do first)*

### Root cause (confirmed)
`enemy.cpp:217-241`: after a 0.45 s windup (`kWindup`, `:202`) each burst shot calls `PlayerDamage(w, s.damage)` **unconditionally** while LOS holds — the beam re-aims at the player's live position every shot. The `aimBeam` telegraph is cosmetic; if you can see it, it hits you. 3 shots × 8 damage with perfect tracking.

### Fix (decided)
Freeze the aim when the attack starts; make the shots a ray test the player can sidestep; give a full second to move; lower the damage.

- `enemy.h`: add `Vector2 aimDir{};` to `Enemy` (doc comment: frozen at attack start — the dodgeable-beam mechanic).
- `enemy.cpp` `UpdateZelfscanner`:
  - `kWindup` `0.45f` → `1.0f`.
  - Where it enters attack (`BeginAttack(e, 3)` at `:211`), set `e.aimDir = Vector2Normalize(Vector2Subtract(w.player.pos, e.pos));` — never update it again during the attack.
  - Replace the unconditional `PlayerDamage(w, s.damage)` (`:237`) with a corridor test against the frozen ray:
    ```cpp
    const Vector2 to = Vector2Subtract(w.player.pos, e.pos);
    const float along = Vector2DotProduct(to, e.aimDir);
    const float perp2 = Vector2DotProduct(to, to) - along * along;
    const float halfW = kPlayerRadius + 0.12f;
    if (along > 0.0f && along <= s.attackRange + 1.0f && perp2 <= halfW * halfW && los) {
        PlayerDamage(w, s.damage);
    }
    ```
    (fire the sound and `e.beam` either way — a miss should still be loud).
  - Keep the existing LOS-break cancel at `:220` (ducking behind a shelf still resets it).
- `kStats` zelfscanner row (`enemy.cpp:16`): damage `8` → `5`.
- `render.cpp`: grep `aimBeam` / `beam` — both beams are currently drawn toward the live player; draw them from `e.pos` along `e.aimDir` instead, length `map.RayToWall(e.pos, e.aimDir, stats.attackRange + 1.0f)`, so the telegraph shows the actual frozen line you must leave.

**Files:** `enemy.h` · `enemy.cpp` · `render.cpp`
**Test:** stand still in the beam → hit ~every burst; strafe one tile during the 1 s windup → the burst visibly misses.

## B — More ammo on the map  *(DECIDED)*

Level 1 (`map.cpp` `kLevel`) has four `l` (labels, +20) pickups. Add **four more** on floor tiles (`.`), one per area: near the checkouts, mid-aisles, the freezer approach, and the magazijn. Executor picks exact tiles; keep them off the player spawn lane so they are earned. `kStartAmmo` stays 40.

**Files:** `map.cpp`
**Test:** count 8 `l` characters; walk two of them, ammo rises +20 each, capped at 200.

## C — Esc on the web build does nothing  *(DECIDED — exact diff)*

### Root cause (confirmed)
raylib's web platform never returns `true` from `WindowShouldClose()` (its web main loop just `emscripten_sleep`s), so the exit key is dead; additionally Chrome swallows the first Esc to release pointer lock. The page hint ("Esc quits the run") is wrong. Desktop behaviour (Esc quits, per the original spec) must not change.

### Fix (decided — apply verbatim)
`claude/src/game.cpp`, in `GameUpdate` directly after the `AudioUpdate` call, and replacing the `Title` case:

```cpp
#if defined(__EMSCRIPTEN__)
    // In a browser there is no quitting: raylib's web main loop never honours the
    // exit key, and while the mouse is captured Chrome eats the first Esc to
    // release pointer lock. So Esc abandons the run and returns to the title.
    if (g.state != GameState::Title && IsKeyPressed(KEY_ESCAPE)) {
        g.state = GameState::Title;
        EnableCursor();
        return;
    }
#endif

    switch (g.state) {
        case GameState::Title: {
            // Esc is not "any key": on the web it just brought us here.
            const int key = GetKeyPressed();
            if ((key != 0 && key != KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                Restart(g);
            }
            break;
        }
```

`claude/src/main.cpp:21`: comment becomes `// Esc quits on desktop; on web it returns to the title (game.cpp)`.
`web/index.html:27`: hint becomes `Esc releases the mouse &mdash; Esc again returns to the title screen.`

**Files:** `game.cpp` · `main.cpp` · `web/index.html`
**Test (web):** start a run, press Esc (twice if mouse captured) → title screen; any other key starts a fresh run; Esc on the title does nothing.

> **Checkpoint after A–C:** commit each task separately on `main`, run `./web/build.sh`, verify locally (script in the acceptance section), then ask Ben to deploy — the deploy command needs his approval/terminal: `npx wrangler pages deploy web/dist --project-name=ah-hell-aisle`. Then branch `feature/expansion` for the rest.

## D — Upstream the web shaders, drop the sed  *(DECIDED)*

`web/build.sh:38` currently seds `#version 330` → `#version 300 es` + `precision highp float;` into a copy of the sources — an arena-era workaround, unnecessary now that `claude/` is editable. In `render.cpp`, prefix the three shader literals with a macro instead:

```cpp
#if defined(__EMSCRIPTEN__)
#define GLSL_HEADER "#version 300 es\nprecision highp float;\n"
#else
#define GLSL_HEADER "#version 330\n"
#endif
// then: const char* kWorldVS = GLSL_HEADER R"(
// in vec3 vertexPosition; ...  — i.e. drop the #version line from each literal
```

Remove the sed and its grep guard from `build.sh` (keep the source copy step or compile `claude/src` directly — copying is no longer needed at all; simplify to taste). Native build must still compile and render identically.

**Files:** `render.cpp` · `web/build.sh`

## E — Extend the art pipeline  *(DECIDED: procedural, via gen_assets.py)*

Read `tools/gen_assets.py` and `assets/MANIFEST.md` first; match the existing palette, cell sizes and sheet layouts (enemy sheets are 6 frames: idle, walk, attack, die×3; weapons are 3 frames). **Only add generator functions — do not modify existing ones.** After regenerating, `git status` must show only *new* files in `assets/`; existing images stay byte-identical.

New sprites: `weapon_statiegeldkanon.png`, `weapon_vuurwerkpijl.png` (3-frame sheets) · `enemy_beveiliger.png` (security guard, 6-frame) · `enemy_bedrijfsleider.png` (the manager-boss, 6-frame, larger cell is fine) · `pickup_flessen.png` (bottle-crate ammo) · `pickup_vuurwerk.png` (fireworks ammo) · `pickup_statiegeldkanon.png`, `pickup_vuurwerkpijl.png` (weapon pickups) · `proj_vuurwerkpijl.png` (small rocket). Update `MANIFEST.md`.

**Files:** `tools/gen_assets.py` · `assets/` (additions only) · `assets/MANIFEST.md`

## F — Two new weapons  *(DECIDED)*

Slots 3 and 4, following the slot-1/2 patterns in `player.cpp` end to end (cooldown gate, 3-frame anim, `AlertEnemies` noise, HUD, sounds).

| | **3 — Statiegeldkanon** (scattergun) | **4 — Vuurwerkpijl** (rocket) |
|---|---|---|
| Type | hitscan, 7 pellets/shot (loop `FirePrijspistool`'s ray per pellet) | projectile (reuse `Projectile` with a new kind; flat flight, no arc) |
| Damage | 6 per pellet | 80 at blast centre → 20 at edge, linear over radius 1.6; **damages the player too** |
| Spread / speed | ±0.09 rad per pellet | 10 u/s, launch height 0.6 |
| Cooldown / anim | 0.9 s / 0.3 s | 1.2 s / 0.35 s |
| Ammo | *flessen*: start 0, cap 50; `f` pickup +8 | *vuurwerk*: start 0, cap 20; `v` pickup +4 |
| Acquired via | `g` map pickup → weapon + 12 flessen | `p` map pickup → weapon + 4 vuurwerk |

Implementation notes (all decided):
- `WeaponId` grows to 4; `Player` gets `bool hasWeapon[4]` (slots 1-2 true from start) and per-type ammo (labels stays, add flessen + vuurwerk); keys `KEY_THREE`/`KEY_FOUR` select only if owned; add a `WeaponSwitch` click sound on any successful switch.
- `Projectile` gains `enum kind {SoupCan, Rocket}` + an `ownerIsPlayer` flag; `ProjectilesUpdate` explodes rockets on wall/enemy/lifetime: apply radial damage to enemies via `EnemyHurt` and to the player via `PlayerDamage`, add `w.shake`, spawn the `Explosion` sound and a brief flash (a scaled billboard or the muzzle-flash tint — executor's call, it's cosmetic).
- Weapon pickups spawn like other pickups (`world.cpp` switch) but set `hasWeapon` + grant ammo + auto-select + `WeaponUp` fanfare; picking up an owned weapon just grants the ammo.
- HUD `AMMO` shows the **selected** weapon's reserve; `-` for the stokbrood.
- Map placement: `g` reachable mid-level-1 (defended); `p` first appears in level 2.
- Assets: `Assets::weapon[2]` → `[4]`, `pickup[5]` → `[9]`, plus `rocketTex`; extend the `Grab` list and the enum-indexed draw code in `render.cpp`/`hud.cpp`.

**Files:** `player.h/cpp` · `config.h` · `enemy.h/cpp` (projectile) · `world.cpp` · `map.cpp` · `assets.h/cpp` · `render.cpp` · `hud.cpp` · `audio.h/cpp`

## G — Two new enemies  *(DECIDED)*

Extend `EnemyKind`, `kStats`, the `world.cpp` spawn switch (`4`, `5`), `Assets::enemy[3]` → `[5]`, and add two update functions in `enemy.cpp` composed from the existing three.

**Beveiliger** (possessed security guard — the advancing shooter): health 80, speed 2.6, radius 0.30, sight 17, range 12, cooldown 1.8 s, damage 10, spriteSize 1.35. Behaviour: closes to ~6 units, vakkenvuller-style sidestep while cooling down, then a **frozen-ray single shot** exactly like task A's mechanic with a 0.5 s windup — reuse `aimDir` and the corridor test (shared helper encouraged).

**De Bedrijfsleider** (the manager — level-3 boss): health 500, speed 3.2, radius 0.5, sight 20, spriteSize 1.9. Two alternating timed phases: **charge** (winkelwagen logic, contact 25 damage, 1.2 s cooldown) and **barrage** (stands, throws a fan of 3 soup cans at −15°/0°/+15°). Spawns via `5` in the level-3 magazijn, guarding the exit. **On death he drops the keycard** (push a `Keycard` pickup at his position — level 3's grid has no `k`). Alert + death get their own sounds; give him a HUD intro message when first sighted (e.g. `"DE BEDRIJFSLEIDER: 'WIJ SLUITEN NOOIT.'"`).

Difficulty escalation is level-authored, not a setting (locked): level 1 ≈ current 24 enemies + 2 beveiligers in the late areas; level 2 ≈ 30 with beveiligers regular; level 3 ≈ 36 heavier mix + the boss. No difficulty menu.

**Files:** `enemy.h/cpp` · `world.cpp` · `map.cpp` · `assets.h/cpp` · `render.cpp` · `audio.h/cpp`

## H — Three levels + progression  *(DECIDED)*

- Grid stays fixed 40×40. Replace `kLevel` with three `LevelDef`s in `map.cpp`:
  ```cpp
  struct LevelDef {
      const char* const* rows;          // [Map::H]
      Zone (*zoneFor)(int x, int y);    // per-level zone rectangles, like map.cpp:79
      const char* intro;                // HUD message on entry
  };
  Map LoadLevel(int level, std::vector<Spawn>& spawns);
  ```
- **Level 1** — the current store, unchanged except tasks B/F/G additions. **Level 2 — "Distributiecentrum"**: magazijn-textured racking in long parallel runs, open sightlines (turret + beveiliger country), freezer corner. **Level 3 — "Laadperron"**: freezer/magazijn mix, tighter, boss arena in front of the final dock. Author both in level-1's style: sealed perimeter, keycard defended and far from spawn, `K` door gating the exit area, one `X`, ≥2 each of `a r l b`, ≥1 `f`/`v` from level 2 on.
- `World` gains `int level`; touching `X` on levels 1-2 loads the next level instead of setting `escaped` (keep `escaped` for level 3). Player **carries** health/armour/ammo/weapons across levels; keycard resets; `kills`/`totalEnemies`/`elapsed` accumulate across the run (end-screen stats are per-run).
- `R` restarts the **current level** with the loadout you *entered* it with — snapshot the carried state at level entry and restore it (death must not soft-lock a run that entered level 3 with 4 health; that is accepted: Doom rules).
- Geometry: `main.cpp:19` bakes once. Add `RenderRebuild(const Map&)` in `render.cpp` (unload the baked meshes, re-run the bake — the pieces already exist in `RenderInit`/`RenderShutdown`) and call it on every level (re)load.
- Level-complete: `LevelDone` jingle + intro message of the next level.

**Files:** `map.h/cpp` · `world.h/cpp` · `game.cpp` · `player.h` (snapshot struct) · `render.h/cpp` · `main.cpp`

## I — Sounds and music  *(DECIDED: procedural, zero files — matches audio.cpp's charter)*

All synthesised via the existing `Render(seconds, fn)`; extend the `Sfx` enum with (executor tunes the oscillators, names are locked): `AlertCart, AlertStocker, AlertScanner, AlertGuard, AlertBoss` (one bark when each leaves Idle — trigger where `Notices` flips state, and in `EnemyHurt`'s wake-up path), `CartRattle` (retriggered every ~0.8 s while a winkelwagen chases within earshot, use `PlaySfxAt`), `GuardShot, Scattergun, RocketLaunch, Explosion, DeathGuard, DeathBoss, WeaponUp, WeaponSwitch, LevelDone`.

**Music** (the new part): a tiny step-sequencer *inside* the render lambda — pattern arrays of note frequencies stepped at a fixed BPM (bass square + noise hats + a kick thump), rendered once into ~24 s `Sound` loops. Four tracks: one per level (same sequencer, different tempo/key/pattern — level 1 sparse dread, level 2 driving, level 3 fast) and a near-static title drone. Loop them exactly like `gHum` (`audio.cpp:190-203`): retrigger on a timer slightly shorter than the length, with edge fades baked in so the seam hides. Music volume ~0.25 under the SFX. `AudioUpdate` gains the current track (title vs level index); **`M` toggles music** (input in `game.cpp`, HUD message "MUZIEK UIT/AAN"). The refrigeration hum stays, layered under everything.

**Files:** `audio.h/cpp` · `enemy.cpp` (bark/rattle triggers) · `game.cpp` (M key) · `claude/README.md` (controls)

## J — Docs, verify, ship

- Update `claude/README.md`: new controls (3/4, M), the three levels, the two new enemies/weapons, web build pointer.
- Full acceptance below; merge `feature/expansion` → `main`; rebuild `./web/build.sh`; ask Ben to deploy (command at checkpoint above — it requires his approval).

## Sequencing

1. **A** kassa fix → 2. **B** ammo → 3. **C** web Esc — each committed on `main`; checkpoint: web rebuild + Ben deploys.
4. Branch `feature/expansion`: **D** shaders → **E** art → **F** weapons → **G** enemies → **H** levels → **I** audio/music → **J** docs+verify+merge. E must precede F/G (they load the sprites). Commit small; F/G/H are cross-cutting — split into compiling sub-commits.

## Decisions — locked ✅

- Scope: 3 levels total, +2 weapons, +2 enemies (one regular, one boss). Difficulty = level-authored counts/mix; **no difficulty menu**.
- Zelfscanner: damage 5, windup 1.0 s, frozen `aimDir` + corridor test (halfwidth `kPlayerRadius + 0.12`), burst unchanged, telegraph beam drawn along the frozen ray. Beveiliger reuses the same mechanic (0.5 s windup).
- All numbers in tasks A/F/G as written; executor may fine-tune ±20 % for feel, nothing more, and notes deviations in the completion note.
- Music/SFX are procedural in `audio.cpp` — no audio files, no new deps. `M` mutes music only.
- New art only via `tools/gen_assets.py`; existing `assets/` files stay byte-identical (additions only).
- Rockets self-damage. Level-3 keycard drops from the boss. Carry-over + per-level `R` snapshot as in task H. Grid fixed at 40×40.
- Web: keep `-sASYNCIFY` and `-sGROWABLE_ARRAYBUFFERS=0` (Chrome rejects WebGL uploads from resizable heap buffers — removing it black-screens the game). Esc on web = back to title (desktop Esc still quits).
- `runner-ups/` untouched, always. Deploys are Ben-only (permission-gated): `npx wrangler pages deploy web/dist --project-name=ah-hell-aisle`.

## Acceptance / pre-merge checklist

- [ ] Native build clean (`cmake … && cmake --build build`) from `claude/`; game runs.
- [ ] `./web/build.sh` clean; serve `web/dist` on :8080; headless check passes (below): no `pageerror`, no `SHADER` warnings, non-black canvas.
- [ ] Kassa dodge test (task A) and ammo test (task B) pass.
- [ ] Web Esc test (task C) passes; desktop Esc still quits.
- [ ] `git status`: nothing under `runner-ups/`; `assets/` shows only new files after regen.
- [ ] Full run: level 1 → 2 → 3, loadout carries, keycard resets, boss drops the card, dock exit → victory screen with cumulative stats; death → GESLOTEN → `R` restarts the current level with the entry loadout.
- [ ] Kill each new enemy with each new weapon; get killed by both; rocket splash hurts you point-blank.
- [ ] Every new Sfx audible in context; each level has its own music loop; `M` mutes/unmutes music only; title drone plays on the title.
- [ ] `claude/README.md` updated; commits small with clear messages; `feature/expansion` merged.

Headless verify (from handover 006, adapt keys as needed) — `npm i puppeteer-core` somewhere disposable, then:

```js
const puppeteer = require('puppeteer-core');
(async () => {
  const b = await puppeteer.launch({ executablePath:
    '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
    headless: 'new', args: ['--mute-audio', '--enable-unsafe-swiftshader'] });
  const p = await b.newPage();
  p.on('pageerror', e => console.log('[pageerror]', e.message));
  p.on('console', m => /SHADER|ERROR|WARNING/.test(m.text()) && console.log(m.text()));
  await p.goto('http://localhost:8080', { waitUntil: 'networkidle2' });
  await p.click('#start'); await new Promise(r => setTimeout(r, 6000));
  await p.keyboard.press('Enter'); await new Promise(r => setTimeout(r, 2000));
  await p.screenshot({ path: 'verify.png' });   // eyeball: textured world, HUD
  await p.keyboard.press('Escape'); await new Promise(r => setTimeout(r, 1000));
  await p.screenshot({ path: 'verify_esc.png' }); // eyeball: title screen again
  await b.close();
})();
```

---
**On completion:** update `.handovers/handover_log.md` — set row 007 to ✅ done and fill the
Completed date. If you could not finish, set 🔄 in-progress and append a note row describing
exactly which tasks (A–J) remain and where you stopped.
