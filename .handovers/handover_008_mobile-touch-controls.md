# Handover 008 — Mobile touch controls for the web build

**Branch:** `feature/mobile-controls` off `master` (note: the repo branch was renamed `main` → `master` on 2026-07-14), merged back to `master` when the acceptance checklist passes.
**Author:** session 2026-07-14 (the session that set up the GitHub remote + deploy docs)  ·  **Status:** pending
**Scope:** make the deployed web build (https://ah-hell-aisle.pages.dev) playable on a phone. Controls decided with Ben: **tap = fire/use**, **horizontal swipe = weapon switch**, **accelerometer tilt = aim (steering-wheel roll)**, **virtual stick on the left half = movement**. Desktop and native behaviour must not change. Ben originally sequenced this before handover 007, but 007's execution started while this doc was being authored (its tasks A–C are on `master` as of `6cdd040`) — so this doc is written **order-independent**: every 007 overlap is called out inline. Check the log for 007's status first; do not touch 007's scope here.

## 0. Shared context (read first)

Repo root `/…/random_stuff/`. Read `CLAUDE.md` (repo root) first — never touch `runner-ups/`. The game is C++17 + raylib 6.0; the web build wraps it with Emscripten (ASYNCIFY main loop) via `web/build.sh` into `web/dist/`, shell page `web/index.html`.

| Concern | Where |
|---|---|
| All player input (aim/move/fire/use/weapons) | `claude/src/player.cpp` — `UpdateDoors` (KEY_E), `UpdateWeapon` (select keys + fire condition), `UpdateMovement` (mouse yaw + WASD). Line numbers omitted on purpose: 007's task F rewrites this file — anchor on the function names. |
| State-screen input (title/dead/escaped restart) | `claude/src/game.cpp` (`GameUpdate` — Title/Dead/Escaped cases; already carries 007's web-Esc rewrite, commit `6cdd040`) |
| Player struct, `WeaponId` | `claude/src/player.h` (2 weapons at authoring time; 4 + `hasWeapon[]` once 007 task F lands) |
| Tuning constants (`kMouseSens` 0.0024 rad/px, `kTurnSpeed` 120°/s, `kMoveSpeed` 3.5) | `claude/src/config.h:17-19` |
| Native build file list (new .cpp must be added) | `claude/CMakeLists.txt:28-39` |
| Web shell page (all JS goes here) | `web/index.html` — start overlay `#start`, canvas `#canvas` |
| Web compile (globs `src-web/*.cpp` — picks up new files automatically) | `web/build.sh:52` (**keep `-sGROWABLE_ARRAYBUFFERS=0`**) |

Build & verify commands:

```sh
# native (from claude/)
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/ah_hell_aisle
# web (from repo root)
./web/build.sh && python3 -m http.server 8080 -d web/dist
```

Two facts that shaped the design (do not re-derive):

- **Aim is yaw-only.** `UpdateMovement`'s `GetMouseDelta().x` line is the entire look system — there is no pitch look anywhere. Accelerometer therefore drives *turning*, steering-wheel style.
- **raylib's Emscripten backend maps single touches to mouse position**, so an unguarded `GetMouseDelta()` would spin the view whenever a finger drags. Task 2 gates it behind `touchMode`.

## 1 — C++ input shim: `web_input.h` / `web_input.cpp`  *(DECIDED)*

New pair in `claude/src/`, compiled by **both** builds (add `src/web_input.cpp` to `CMakeLists.txt`; `build.sh` picks it up via its glob). Native never calls the setters, the struct stays zeroed, every read is a no-op — so game code reads it unconditionally, no `#ifdef`s outside this file.

`web_input.h`:

```cpp
#pragma once
// Input injected from the browser shell (web/index.html) on touch devices.
// Native builds compile this too; nothing ever writes it there.
struct WebInput {
    bool  touchMode = false;   // set once on first touch: disables mouse-look
    float turnRate  = 0.0f;    // rad/s from tilt; JS refreshes it per sensor event
    float yawDelta  = 0.0f;    // rad, drag-look fallback; consumed per frame
    float moveX = 0.0f;        // virtual stick, screen-right strafe, |v| <= 1
    float moveY = 0.0f;        // virtual stick, screen-up = forward,  |v| <= 1
    bool  fireDown    = false; // held state
    bool  firePressed = false; // edge — survives a sub-frame tap; consumed
    bool  usePressed  = false; // edge; consumed
    int   weaponStep  = 0;     // accumulated swipe steps (±1 each); consumed
};
extern WebInput gWebInput;
float WebConsumeYawDelta();
bool  WebConsumeFirePressed();
bool  WebConsumeUsePressed();
int   WebConsumeWeaponStep();
```

`web_input.cpp`: define `gWebInput`, the four consume helpers (return current value, zero it), and — inside `#if defined(__EMSCRIPTEN__)`, including `<emscripten.h>` — the JS-facing setters, all `extern "C"` + `EMSCRIPTEN_KEEPALIVE`:

```cpp
void web_set_touch_mode(int on);           // gWebInput.touchMode = on
void web_set_turn_rate(float radPerSec);   // overwrite
void web_add_yaw(float rad);               // accumulate into yawDelta
void web_set_move(float x, float y);       // overwrite both
void web_set_fire(int down);               // fireDown = down; if (down) firePressed = true
void web_press_use(void);                  // usePressed = true
void web_cycle_weapon(int step);           // weaponStep += step
```

JS calls them as `Module._web_set_fire(1)` etc. — no `ccall` needed for int/float args. Single-threaded (ASYNCIFY yields to the event loop between frames), so plain writes are safe.

**Files:** `claude/src/web_input.h` · `claude/src/web_input.cpp` · `claude/CMakeLists.txt`

## 2 — Game integration  *(DECIDED — exact edits)*

All reads unconditional; native is unaffected because the struct is all zeros there.

`player.cpp` `UpdateMovement`:

```cpp
// the GetMouseDelta yaw line becomes — touch devices must not double-steer
// via raylib's touch→mouse mapping
if (!gWebInput.touchMode) p.yaw += GetMouseDelta().x * kMouseSens;
p.yaw += gWebInput.turnRate * dt + WebConsumeYawDelta();
```

and merge the stick into the wish vector — analog magnitude scales speed, keyboard keeps full speed:

```cpp
wish = Vector2Add(wish, Vector2Scale(p.forward(), gWebInput.moveY));
wish = Vector2Add(wish, Vector2Scale(p.right(),  gWebInput.moveX));

const bool moving = Vector2Length(wish) > 0.01f;
if (moving) {
    const float strength = fminf(1.0f, Vector2Length(wish));
    const Vector2 step = Vector2Scale(Vector2Normalize(wish), kMoveSpeed * dt * strength);
    // …existing per-axis slide unchanged
```

`player.cpp` `UpdateWeapon`: after the weapon-select key lines add the swipe cycle, and extend the fire condition. The cycle depends on whether 007's task F has landed — check `player.h` for `hasWeapon`:

```cpp
if (int step = WebConsumeWeaponStep()) {
    // pre-007-F (WeaponId still has 2 entries):
    constexpr int kWeaponCount = 2;   // 007 task F upgrades this to owned-slot skipping
    p.weapon = (WeaponId)((((int)p.weapon + step) % kWeaponCount + kWeaponCount) % kWeaponCount);
    // post-007-F: instead step to the next/previous slot with hasWeapon[slot], wrapping —
    // that ownership-aware cycle is the intended end state either way.
}
…
const bool firing = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                    IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                    gWebInput.fireDown || WebConsumeFirePressed();
```

(`firePressed` is what makes a faster-than-one-frame tap still fire.)

`player.cpp` `UpdateDoors`: `if (IsKeyPressed(KEY_E) || WebConsumeUsePressed()) {`

`game.cpp` — tap advances every state screen (each state is the sole consumer of the edge that frame; when Playing, `UpdateWeapon` consumes it — `PlayerUpdate` early-returns before `UpdateWeapon` when dead, so there is no double-consume path). 007's task C already rewrote the Title case (commit `6cdd040`), so amend the conditions as they stand:

- Title case: append `|| WebConsumeFirePressed()` to the restart condition (currently `(key != 0 && key != KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)`).
- Dead and Escaped cases: `if (IsKeyPressed(KEY_R) || WebConsumeFirePressed()) Restart(g);`

**Files:** `player.cpp` · `game.cpp`

## 3 — JS touch layer in `web/index.html`  *(DECIDED)*

Everything lives in the shell page; keep it dependency-free vanilla JS. Gate the whole layer on `('ontouchstart' in window) || navigator.maxTouchPoints > 0` — desktop gets zero new DOM/listeners. Attach listeners in `Module.onRuntimeInitialized` (extend the existing `window.Module = { canvas }`); guard every `Module._web_*` call behind that ready flag. Use Touch Events (not Pointer Events) and track touches by `identifier`. Add CSS `touch-action: none` on `.frame` plus `e.preventDefault()` in handlers (kills scroll/zoom/double-tap-zoom), and `user-select: none`.

Zones, split at 50% of the frame width — by where the touch **starts**:

**Left half — floating virtual stick.** touchstart anchors the stick base at the touch point; touchmove sets the vector: clamp to 64 CSS px radius, 8 px deadzone, full speed at 48 px. Screen-up = forward. Send `Module._web_set_move(dx/48, -dy/48)` clamped to |v| ≤ 1; on touchend `_web_set_move(0, 0)`. Draw base + nub as two absolutely-positioned circular divs (semi-transparent, `pointer-events: none`), hidden when no left touch is down.

**Right half — tap / hold / swipe state machine** (per touch, thresholds locked):

| Event | Transition |
|---|---|
| touchstart | record `t0`, `p0`; state = PENDING; start a 100 ms timer |
| timer fires, still PENDING | state = FIRING: `_web_set_fire(1)` + `_web_press_use()` |
| touchmove while PENDING, moved > 12 px | state = SWIPE_CANDIDATE (or LOOK in the no-motion fallback, below) |
| touchend from FIRING | `_web_set_fire(0)` |
| touchend from PENDING (quick tap) | `_web_set_fire(1)` then `_web_set_fire(0)` + `_web_press_use()` — the `firePressed` edge lands one shot |
| touchend from SWIPE_CANDIDATE | if duration ≤ 350 ms, \|dx\| ≥ 48 px and \|dx\| ≥ 2·\|dy\|: `_web_cycle_weapon(dx > 0 ? 1 : -1)` (swipe right = next); else nothing |

A slow drag (>100 ms to move 12 px) commits as FIRING and can no longer become a swipe — accepted. First touch anywhere also calls `_web_set_touch_mode(1)` once.

**Accelerometer aim** (steering-wheel roll → turn rate):

- Permission: inside the existing `#start` click handler (it is the required user gesture), `if (typeof DeviceMotionEvent?.requestPermission === 'function')` await it (iOS 13+); non-iOS just `addEventListener('devicemotion', …)`. Also best-effort in the same handler: `document.documentElement.requestFullscreen()` and `screen.orientation.lock('landscape')`, both in try/catch — iPhone Safari supports neither; carry on.
- Tilt from gravity, not Euler angles (deviceorientation gimbal-locks near landscape). Per `devicemotion` event take `g = accelerationIncludingGravity`, EMA-smooth with α = 0.25. Capture calibration `g0` as the average of the first ~20 samples after start; recapture on `orientationchange`. Signed screen-roll tilt, sign-convention-proof (a global negation of the vector cancels in the products):

  ```js
  const tilt = Math.atan2(g0.x * g.y - g0.y * g.x, g0.x * g.x + g0.y * g.y);
  ```

- Map to rate: deadzone 3°, linear to full at 25°, max 150°/s. `rate = sign(tilt) * min(1, max(0, (|tilt|−3°)/(25°−3°))) * 150°/s * SIGN`, sent as radians via `_web_set_turn_rate(rate)`. `SIGN = (screen.orientation.angle === 270 ? -1 : 1)` — physical check is an acceptance item; if tilting right turns left on device, flip the base sign once.
- Watchdog: if no `devicemotion` event for 500 ms, send `_web_set_turn_rate(0)`.

**Fallback when motion is unavailable or denied:** enable drag-look on the right half — in the state machine, a moved-PENDING touch becomes LOOK instead of SWIPE_CANDIDATE; each touchmove sends `_web_add_yaw(dxPx * 0.0045)`. Swipe classification still runs at touchend (the small view nudge a swipe causes in fallback mode is accepted). Show a one-line hint ("motion aim off — drag to look").

**Page furniture:** on touch devices the start overlay reads "▶ tap to start" and the hint line becomes "tilt to steer · left thumb moves · tap fires · swipe switches weapon". Desktop text unchanged.

**Debug seams (keep, they're the desktop test path):** `window.__mob = { setGravity(x, y, z), state() }` — `setGravity` feeds the same pipeline as the sensor; `state()` returns the current tilt/rate/stick values.

**Files:** `web/index.html`

## 4 — Docs  *(DECIDED)*

- `claude/README.md`: add a "Mobile (web)" controls paragraph (the four gestures, motion-permission note).
- Repo `README.md`: no change needed (deploy flow already documented).

## Sequencing

1. Task 1 (shim) + task 2 (integration) + CMake entry → native build green, desktop behaviour identical (keyboard/mouse regression by hand).
2. Task 3 (JS layer) → `./web/build.sh`, test in Chrome DevTools device emulation.
3. Real-device pass on the preview URL — the deploy itself is **Ben-only**: ask him to run `npx wrangler pages deploy web/dist --project-name=ah-hell-aisle --branch=mobile-preview` (a non-`main` `--branch` value yields a preview URL, production untouched).
4. Task 4 (docs), acceptance checklist, merge `feature/mobile-controls` → `master`.

## Decisions — locked ✅

- Controls: tap = fire (hold = autofire) + use; horizontal swipe right/left = next/prev weapon; tilt roll = turn (yaw only — the game has no pitch); floating virtual stick on the left half = move. Chosen by Ben 2026-07-14.
- Order vs 007: originally "008 first" (Ben), overtaken by events — 007 started executing during authoring. This doc works from any point in 007's progression; the inline "pre/post-007-F" forks are the only order-sensitive spots. 007 was amended by the author where they overlap; the 008 executor does not edit 007.
- Injection is exported-function based (`web_input` shim), **not** synthetic DOM events — raylib-web's pointer-lock and touch→mouse quirks make synthetic events unreliable. Mouse-look is gated off by `touchMode` for the same reason.
- Tilt source is `devicemotion` gravity with runtime calibration (formula above); deviceorientation Euler angles rejected (gimbal trouble in landscape). Aim is rate-based with deadzone — thresholds as written, executor may tune any JS constant ±30 % for feel and notes deviations in the completion note.
- No on-screen fire/weapon buttons; no pitch look; no gamepad; portrait mode just letterboxes (no CSS rotation hack). Motion-denied fallback = drag-look, nothing fancier.
- Weapon cycle is count-based over the current 2 weapons; ownership-aware skipping is 007's job (already noted there).
- Desktop web and native must be pixel-for-pixel behaviourally unchanged. `runner-ups/` untouched, always. Deploys are Ben-only.

## Acceptance / pre-merge checklist

- [ ] Native build clean from `claude/`; keyboard/mouse play unchanged (turn, WASD, fire, E, 1/2, R).
- [ ] `./web/build.sh` clean; desktop Chrome on :8080 plays unchanged; no `pageerror` in console.
- [ ] DevTools device emulation (iPhone profile): tap fires (muzzle flash + sound); hold autofires; quick tap lands exactly one shot; stick walks all 8 directions with head-bob; swipe right/left switches weapon sprite both ways; tap starts the game from the title and restarts from GESLOTEN/escape screens.
- [ ] `window.__mob.setGravity(…)`: tilt within ±3° holds still; beyond it turns, full rate by 25°; `__mob.state()` values sane.
- [ ] Desktop (non-touch): no stick divs, no mobile hints, no listeners — DOM identical to before except the script block.
- [ ] Real iPhone via preview deploy (Ben runs the deploy — command in Sequencing): motion permission prompt appears from the start tap; grant → tilting right turns right (else flip `SIGN` once and note it); deny → drag-look works and hint shows.
- [ ] `git status` clean of `runner-ups/`; commits small; branch merged to `master`.

---
**On completion:** update `.handovers/handover_log.md` — set row 008 to ✅ done and fill the
Completed date. If you could not finish, set 🔄 in-progress and append a note row describing
what remains (which task, which checklist items).
