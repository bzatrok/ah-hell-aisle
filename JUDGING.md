# Judging — AH: Hell Aisle bake-off

Four models, one spec, four folders: `opencode/`, `vibe/`, `claude/`, `codex/`.
Each got the identical `HANDOVER.md`, one shot, no coaching.

## Gates (pass/fail — a failure here caps the score)

| Gate | Test |
|---|---|
| **Builds** | `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` from the folder, clean, first try. |
| **Runs** | `./build/ah_hell_aisle` opens a window and reaches gameplay. |
| **Completable** | The §10 checklist in `SPEC.md` can be walked end to end without a crash. |

A model that does not build scores zero on everything below. Note *how far* it got
anyway — a beautiful engine that misses a semicolon is a different failure from a
build that works and plays like mud.

## Scored (1–5 each)

| Weight | Criterion | What a 5 looks like |
|---|---|---|
| ×3 | **Spec faithfulness** | Every mandatory element present and behaving as written. Three enemies genuinely distinct. Keycard gate works. Non-goals respected — no ECS, no level editor, no test suite. |
| ×3 | **Does it feel good** | Ninety seconds in, you're still playing. Movement is responsive, the gun has weight, enemies threaten you differently, the aisles are worth exploring. |
| ×2 | **Code quality** | You could hand this to a colleague. Clear module boundaries, no 2000-line `main.cpp`, no premature abstraction either. Readable without a map. |
| ×2 | **Correctness under abuse** | Strafe into corners, shoot through shelves, spam the fire key, die on the keycard, restart twice. Does anything break? |
| ×1 | **Theme** | Does it actually feel like an Albert Heijn at 3am, or is it Doom with a blue coat of paint? |
| ×1 | **Judgement calls** | Where the spec left room, did it choose well? Did the README honestly say what it cut? |

## Score sheet

| | opencode | vibe | claude | codex |
|---|---|---|---|---|
| Builds | | | | |
| Runs | | | | |
| Completable | | | | |
| Spec faithfulness ×3 | | | | |
| Feel ×3 | | | | |
| Code quality ×2 | | | | |
| Correctness ×2 | | | | |
| Theme ×1 | | | | |
| Judgement ×1 | | | | |
| **Total /60** | | | | |

## Notes worth writing down while playing

- What did each model reach for architecturally, unprompted?
- Who wrote their own billboard/depth-sort and who leaned on raylib's helpers?
- Who over-engineered, and who under-delivered?
- Which one's `README.md` was honest about what it skipped?
