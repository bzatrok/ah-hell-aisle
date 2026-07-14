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
| 006 | [web-wasm-claude](handover_006_web-wasm-claude.md) | fresh Claude session | WASM build of claude/ game, deployed to Cloudflare Pages | 🔄 in-progress | `web/` | 2026-07-14 | — |
| 007 | [game-expansion](handover_007_game-expansion.md) | fresh session | Kassa fix, web Esc, +2 weapons, +2 enemies, 3 levels, SFX + music | ⬜ pending | `claude/` | 2026-07-14 | — |

> **Arena result (2026-07-14):** `claude/` won and is unlocked for further development.
> The other implementations were archived under `runner-ups/`.

> **006 note (2026-07-14):** built and visually verified in headless Chrome — title screen, textured world, HUD and movement all render; `claude/`/`assets/` untouched. Fixed an initial black screen: emscripten 6 + `ALLOW_MEMORY_GROWTH` backs the heap with a resizable ArrayBuffer, which Chrome rejects for WebGL texture uploads; `-sGROWABLE_ARRAYBUFFERS=0` restores classic growth. Only the deploy remains, pending Ben's approval: `npx wrangler pages deploy web/dist --project-name=ah-hell-aisle`.

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
