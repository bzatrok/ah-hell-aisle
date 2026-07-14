# Handover Log

One handover, four executors. The doc lives at the repo root as
[`HANDOVER.md`](../HANDOVER.md) rather than in this folder, because it is pasted
into four *external* CLIs (opencode, vibe, claude, codex) that are told to read it
at a stable, obvious path. Each row below tracks one model's run of that same doc.

Status legend: ⬜ pending · 🔄 in-progress · ✅ done · ❌ failed to build

| # | Slug | Executor | Goal | Status | Folder | Created | Completed |
|---|------|----------|------|--------|--------|---------|-----------|
| 001 | [ah-hell-aisle](../HANDOVER.md) | opencode | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `opencode/` | 2026-07-13 | — |
| 002 | [ah-hell-aisle](../HANDOVER.md) | vibe | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `vibe/` | 2026-07-13 | — |
| 003 | [ah-hell-aisle](../HANDOVER.md) | claude | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `claude/` | 2026-07-13 | — |
| 004 | [ah-hell-aisle](../HANDOVER.md) | codex | Doom 2 clone in an Albert Heijn, per SPEC.md | ⬜ pending | `codex/` | 2026-07-13 | — |
| 005 | [ah-hell-aisle](../HANDOVER.md) | claude_fable | Doom 2 clone in an Albert Heijn, per SPEC.md | ✅ done | `claude_fable/` | 2026-07-13 | 2026-07-13 |

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
