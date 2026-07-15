# Handover Log

One handover, four executors. The doc lives at the repo root as
[`HANDOVER.md`](../HANDOVER.md) rather than in this folder, because it is pasted
into four *external* CLIs (opencode, vibe, claude, codex) that are told to read it
at a stable, obvious path. Each row below tracks one model's run of that same doc.

Status legend: ⬜ pending · 🔄 in-progress · ✅ done · ❌ failed to build

| # | Slug | Executor | Goal | Status | Folder | Created | Completed |
|---|------|----------|------|--------|--------|---------|-----------|
| 001 | [ah-hell-aisle](../HANDOVER.md) | opencode | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `runner-ups/opencode/` | 2026-07-13 | — |
| 002 | [ah-hell-aisle](../HANDOVER.md) | vibe | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `runner-ups/vibe/` | 2026-07-13 | — |
| 003 | [ah-hell-aisle](../HANDOVER.md) | claude | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `claude/` | 2026-07-13 | — |
| 004 | [ah-hell-aisle](../HANDOVER.md) | codex | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `runner-ups/codex/` | 2026-07-13 | — |
| 005 | [ah-hell-aisle](../HANDOVER.md) | claude_fable | Doom 2 clone in an Albert Heijn, per SPEC.md | ✅ done | `runner-ups/claude_fable/` | 2026-07-13 | 2026-07-13 |
| 006 | [web-wasm-claude](handover_006_web-wasm-claude.md) | fresh Claude session | WASM build of claude/ game, deployed to Cloudflare Pages | ✅ done | `web/` | 2026-07-14 | 2026-07-14 |
| 008 | [mobile-touch-controls](handover_008_mobile-touch-controls.md) | fresh session | Mobile web controls: tap fire/use, swipe weapons, tilt aim, virtual stick | ✅ done | `claude/src` + `web/` | 2026-07-14 | 2026-07-15 |
| 007 | [game-expansion](handover_007_game-expansion.md) | fresh session | Kassa fix, web Esc, +2 weapons, +2 enemies, 3 levels, SFX + music | ✅ done | `claude/` | 2026-07-14 | 2026-07-14 |
| 009 | [korona-parliament-fork](handover_009_korona-parliament-fork.md) | fresh session | Fork: Hungarian Parliament re-theme, regalia weapons, fictional cast | ⬜ pending | branch `korona`: `claude/` + `assets/` + `tools/` | 2026-07-14 | — |
| 010 | [multiplayer-arena](handover_010_multiplayer-arena.md) | same session (design record) | PvPvE deathmatch arena: relay server, netcode, Nachtdienst map | ✅ done | `server/` + `claude/src` + `web/` on `feature/multiplayer-arena` | 2026-07-15 | 2026-07-15 |

> **Arena result (2026-07-14):** `claude/` won and is unlocked for further development.
> The other implementations were archived under `runner-ups/`.

> **006 note (2026-07-14):** built and visually verified in headless Chrome — title screen, textured world, HUD and movement all render; `claude/`/`assets/` untouched. Fixed an initial black screen: emscripten 6 + `ALLOW_MEMORY_GROWTH` backs the heap with a resizable ArrayBuffer, which Chrome rejects for WebGL texture uploads; `-sGROWABLE_ARRAYBUFFERS=0` restores classic growth. Deploy confirmed live in production at https://ah-hell-aisle.pages.dev (source `dcc3cbd`) — closed 2026-07-14.

> **007/008 ordering (2026-07-14, Ben):** **008 runs after 007 completes** — its doc has a hard
> prerequisite gate on row 007 being ✅ and is written against 007's end state (4-slot weapon
> ownership, 3 levels, web Esc). The "if 008 has run first" interop notes inside 007's doc are
> dead branches from an earlier ordering; 007's executor can ignore them.

> **009 ordering (2026-07-14):** 009 also gates on row 007 ✅ (its doc checks for merged
> expansion code on `main`), and is **independent of 008**. It works on the permanent fork
> branch `korona`, which never merges back to `main`.

> **008 note (2026-07-14):** all code and docs done on `feature/mobile-controls`
> (unmerged); native + web builds green; desktop and touch-emulation acceptance pass
> (Puppeteer `hasTouch` run: tap = one shot, hold autofires, swipe cycles owned slots
> both ways with wrap, 8-way stick, tilt deadzone/full-rate/sign checks via `__mob`,
> desktop DOM untouched, zero page errors). One deviation from the doc's exact edit:
> the fire condition's `IsMouseButtonDown` is also gated behind `!touchMode` — raylib
> merges touch state into mouse buttons (rcore.c), so the stick finger would autofire
> otherwise; same rationale as the doc's own `GetMouseDelta` gate. Tap-restart on the
> dead/escaped screens shares the title path's `WebConsumeFirePressed` (verified in
> code; not exercised live in emulation). **Remaining:** the Ben-only real-iPhone pass
> (preview deploy command in the doc's Sequencing §3; flip the JS `SIGN` once if
> tilting right turns left), then merge to `master` and flip this row ✅.

> **008 revision (2026-07-14, Ben's device verdict):** tilt aim is out — on the real
> iPhone it didn't feel right. Right-thumb drag now aims (the old motion-off fallback
> promoted to the only path; permission prompt, gravity pipeline and `SIGN` all gone),
> and the game is landscape-only: portrait raises the new `web_set_paused` flag and a
> "draai je telefoon" overlay, rotating back resumes. Headless touch emulation
> re-verified drag-aim, stick, tap-start and the pause/resume round-trip, zero page
> errors; preview redeployed. **Remaining:** Ben re-tests on the phone, then merge.

> **008 round 2 (2026-07-14, Ben's second device pass):** three fixes. The 16:9 frame
> is now bounded by viewport height too (landscape cut the HUD off the bottom); the
> right thumb gets its own floating stick visual while aiming; and the portrait pause
> that worked in emulation but not on the phone is pinned on Safari serving the *old*
> cached wasm without `web_set_paused` — every asset URL now carries a `?v=<build-id>`
> stamped by `web/build.sh`, and orientation is re-checked on `pageshow`/
> `visibilitychange` for tabs iOS thaws already rotated. Headless suite green
> (frame ≤ viewport, drag-aim, right-stick show/hide, pause round-trip, 0 errors).
> **Remaining:** unchanged — Ben's phone pass, then merge.

> **008 closed (2026-07-15, Ben's go-ahead):** `feature/mobile-controls` merged to
> `master` as `2fac689` (no-ff, matching 007's merge). The headless suite re-ran green
> against the bundle built from that exact commit (BUILD `2fac689`, 0 page errors).
> Production web deploy is a separate Ben-gated step; until it runs, production stays
> on the 007 build and the newest controls live at mobile-preview.ah-hell-aisle.pages.dev.
> (Production deployed later the same day: `2fac689` live at ah-hell-aisle.pages.dev.)

> **010 note (2026-07-15):** built and verified in-session on `feature/multiplayer-arena`
> (unmerged), ten commits: .NET room relay (`server/`, Docker), web_input-style net
> bridge, the Nachtdienst arena, tintable klant sprite, shooter-authoritative PvP +
> host-streamed monsters, join UI + JSON protocol, arena HUD. Two-headless-browser
> suite green end to end (roster, movement, shared trolleys at zero drift, credited
> frag both sides, respawn, host handoff); solo + mobile regressions green.
> **Remaining:** Ben's local two-tab pass, merge call, and the separate hosting round
> (Hetzner + wss) before the arena reaches the public site.

> **010 closed (2026-07-15, Ben waived the local pass — "verified is good enough"):**
> `feature/multiplayer-arena` merged to `master` as `e3f3884` (no-ff, matching 007/008).
> Both suites re-ran green against the bundle built from that exact commit, and the
> deploy went to production: e3f3884 live at ah-hell-aisle.pages.dev, smoke-checked
> (solo boots and renders; ARENA without a relay degrades to "geen relay
> geconfigureerd"). **Still open:** only the hosting round — a public wss relay
> (`server/docker-compose.prod.yml` is ready) plus a `RELAY_URL` rebuild/redeploy,
> which is what turns the public ARENA button on.

## Launch lines

Run each from the repo root, in that model's own CLI:

```
execute this file: HANDOVER.md — you are "opencode", work only in opencode/
execute this file: HANDOVER.md — you are "vibe",     work only in vibe/
execute this file: HANDOVER.md — you are "claude",   work only in claude/
execute this file: HANDOVER.md — you are "codex",    work only in codex/
```

## Rules of the run

- One shot each. No coaching, no follow-ups, no fixing a model's build for it.
- If a model asks a clarifying question, the answer is "the spec is the spec" —
  anything more contaminates the comparison.
- When a model finishes, set its row to ✅ (or ❌ if it does not build) and score it
  in [`JUDGING.md`](../JUDGING.md).
