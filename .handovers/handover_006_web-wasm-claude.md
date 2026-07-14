# Handover 006 — Web/WASM build of `claude/` on Cloudflare Pages

**Branch:** `main` (additive work, one new folder; commit small and often)  ·  **Author:** Claude session 2026-07-14  ·  **Status:** pending
**Scope:** Build a harness in a new top-level `web/` folder that compiles the existing `claude/` competitor game (C++17 + raylib) to WebAssembly with Emscripten and deploys the static result to Cloudflare Pages. The judged source must stay byte-identical: **never edit `claude/` or `assets/`** — the harness copies sources into a gitignored build dir and patches the copy. No Dockerfile (explicitly waived by Ben: static content goes to Cloudflare Pages per his stack rules).

## 0. Shared context (read first)

- This repo is a four-model bake-off arena (see root `CLAUDE.md`). `assets/` is frozen; competitor folders are off-limits for edits. This handover is meta-repo exhibition work: we port `claude/`'s game for browser play without touching the judged code.
- The game: 1280×720 window (`claude/src/config.h:45`), 22 source files in `claude/src/` (11 `.cpp`), standard raylib. Confirmed by author inspection:
  - **Main loop** is a blocking `while (!WindowShouldClose())` in `claude/src/main.cpp:21`. Browsers can't block the main thread → link with `-sASYNCIFY` (raylib's officially supported no-code-change web path). Do NOT restructure the loop.
  - **Assets**: `claude/src/assets.cpp:29` probes `../assets/`, `assets/`, `../../assets/` relative to CWD. Emscripten's virtual FS starts at CWD `/`, where `../assets/` resolves to `/assets/` — so preloading the repo's `assets/` dir at virtual path `assets` works with zero changes. 23 PNGs, nothing else.
  - **Audio** (`claude/src/audio.cpp`): fully procedural via `LoadSoundFromWave` — no audio files, no file I/O anywhere in the codebase. Works on web as-is. Browser autoplay policy keeps the AudioContext suspended until the first user gesture; the shell page's click-to-start overlay guarantees that gesture.
  - **Shaders — the one real incompatibility**: three embedded GLSL strings at `claude/src/render.cpp:32,50,78` start with `#version 330` (desktop GLSL), which will not compile under WebGL. Author verified their bodies use only ES-300-compatible features (`in`/`out`, `texture()`, `mix`, `step`, `exp`, `discard`). Fix is a mechanical transform **on the copied sources only** (§2) plus building raylib for WebGL 2 / OpenGL ES 3.
  - `claude/CMakeLists.txt` is macOS/brew-specific. Do not coerce it through emcmake — the web build compiles the 11 `.cpp` files directly with `em++` (§3).
- Toolchain state on this machine: brew has `raylib 6.0` installed; **`emcc`/`emcmake` are NOT installed** — installing Emscripten is step 1.

| Concern | Path |
|---|---|
| Game loop | `claude/src/main.cpp:21` |
| Asset path probing | `claude/src/assets.cpp:26-37` |
| Shaders to transform (in the copy) | `claude/src/render.cpp:32,50,78` |
| Screen constants | `claude/src/config.h:45-46` |
| New harness (create) | `web/build.sh`, `web/index.html`, `web/.gitignore` |

## 1 — Scaffold `web/`  *(DECIDED)*

Create:

- `web/.gitignore` containing `build/` and `dist/` (raylib clone and all artifacts live under `web/build/`; nothing generated is committed).
- `web/build.sh` — end-to-end build script implementing §2–§4. Idempotent: skip the raylib clone/build if already present.
- `web/index.html` — hand-written shell page (§5). This file is committed; `build.sh` copies it into `web/dist/`.

Commit the scaffold before building.

## 2 — Toolchain + raylib web build  *(DECIDED)*

```sh
brew install emscripten          # provides emcc/em++/emcmake, no emsdk activation needed
git clone --depth 1 --branch 6.0 https://github.com/raysan5/raylib.git web/build/raylib
```

If tag `6.0` doesn't exist, run `git ls-remote --tags https://github.com/raysan5/raylib.git` and use the 6.0.x release tag (match the brew version so web and native behavior stay comparable).

Build raylib for web with **OpenGL ES 3** (required so the `#version 300 es` shaders from §3 compile; raylib's web default is ES2/GLSL 100, which would break them):

```sh
emcmake cmake -S web/build/raylib -B web/build/raylib-build \
  -DPLATFORM=Web -DGRAPHICS=GRAPHICS_API_OPENGL_ES3 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
cmake --build web/build/raylib-build -j
# artifact: web/build/raylib-build/raylib/libraylib.a ; headers: web/build/raylib/src
```

## 3 — Copy sources and apply the shader transform  *(DECIDED — never edit `claude/` in place)*

```sh
rm -rf web/build/src-web && cp -R claude/src web/build/src-web
# GLSL 330 -> GLSL ES 300. highp is mandatory-supported in ES 3.00 fragment shaders,
# and safe in the vertex stage (mediump there could jitter gl_Position math).
sed -i '' 's/#version 330/#version 300 es\'$'\nprecision highp float;/' web/build/src-web/render.cpp
```

Verify: `grep -c "300 es" web/build/src-web/render.cpp` must print `3`, and `git status` must show `claude/` untouched.

## 4 — Compile and link  *(DECIDED)*

Run from the repo root (matters for `--preload-file`):

```sh
mkdir -p web/dist
em++ -std=c++17 -O2 web/build/src-web/*.cpp \
  -I web/build/raylib/src \
  web/build/raylib-build/raylib/libraylib.a \
  -sUSE_GLFW=3 \
  -sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 \
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
  -sALLOW_MEMORY_GROWTH=1 \
  --preload-file assets@assets \
  -o web/dist/game.js
cp web/index.html web/dist/index.html
```

Notes already decided — don't re-derive:
- `-sASYNCIFY` makes the unmodified blocking loop and `SetTargetFPS` (via `emscripten_sleep`) work. If the console ever shows an `unwind`/asyncify stack error, raise `ASYNCIFY_STACK_SIZE`; 131072 is the starting value.
- `--preload-file assets@assets` packages the frozen `../assets/` (from repo root) at virtual `/assets`, which the game's `../assets/` probe resolves to (see §0).
- Output is `game.js` + `game.wasm` + `game.data`; our own `index.html` is the shell (no Emscripten default shell).

## 5 — Shell page `web/index.html`  *(DECIDED)*

Minimal, self-contained, no external resources:

- `<canvas id="canvas" width="1280" height="720" tabindex="0">`, CSS: `max-width: 100%`, `image-rendering: pixelated`, centered on a dark background, `aspect-ratio: 16/9`.
- A click-to-start overlay covering the canvas: on click, hide overlay, define `var Module = { canvas: document.getElementById('canvas') };`, inject `<script src="game.js">`, and `canvas.focus()`. Loading the engine only after the click both satisfies the audio-gesture requirement and gives a clean loading experience.
- A one-line hint under the canvas: "Esc quits the run — refresh the page to restart." (SPEC §4 maps Esc to quit; on web the loop simply ends and the last frame freezes. This is accepted behavior — do not patch around it.)
- Title: "AH: Hell Aisle — claude". Keep styling minimal; this is an exhibition page, not a product.

## 6 — Deploy to Cloudflare Pages  *(DECIDED)*

Precheck auth — this is the only step that can block:

```sh
npx wrangler whoami
```

If not authenticated, **stop and ask Ben** to run `! npx wrangler login` in the session, then continue. Then:

```sh
npx wrangler pages project create ah-hell-aisle --production-branch=main   # first time only
npx wrangler pages deploy web/dist --project-name=ah-hell-aisle
```

Report the resulting `*.pages.dev` URL to Ben. Pages serves `.wasm`/`.data` with workable MIME types; no `_headers` file needed (no threads → no COOP/COEP).

## Sequencing

1. §1 scaffold `web/` → commit.
2. §2 install Emscripten, clone + build raylib web (ES3).
3. §3 copy + transform sources (verify `claude/` untouched).
4. §4 compile/link, assemble `web/dist/`.
5. §5 already committed with §1; verify locally per acceptance below.
6. Commit `build.sh`/`index.html` refinements → §6 deploy → report URL.

## Decisions — locked ✅

- **Emscripten/WASM with `-sASYNCIFY`**, not pixel-streaming, not `emscripten_set_main_loop` refactors. Chosen over Docker+noVNC for UX, cost, and static hosting.
- **Scope: `claude/` only.** No comparison page, no other competitor builds (may come later as a separate handover).
- **Hosting: Cloudflare Pages only**, project name `ah-hell-aisle`. No Dockerfile/.dockerignore — explicitly waived for this static-content harness.
- **`claude/` and `assets/` are never modified.** All patching happens on copies under gitignored `web/build/`. The only transform permitted is the 3-shader `#version` sed of §3.
- **WebGL 2 / `GRAPHICS_API_OPENGL_ES3`** for both raylib and link flags — required by the `#version 300 es` shaders.
- **Bypass `claude/CMakeLists.txt`** for the web build; compile directly with `em++`.
- **raylib pinned to 6.0** (matches brew-installed native version).
- Esc-freezes-page behavior is accepted; documented in the shell page, not worked around.
- Branch: `main`.

## Acceptance / pre-merge checklist

- [ ] `./web/build.sh` runs clean from repo root on a fresh checkout (after `brew install emscripten`) and produces `web/dist/{index.html,game.js,game.wasm,game.data}`.
- [ ] `git status` shows **no changes** under `claude/` or `assets/`, and no build artifacts staged.
- [ ] Local run: `python3 -m http.server 8080 -d web/dist`, open `http://localhost:8080` — click to start, then verify: title/game renders with textures (not flat/black — proves shaders compiled), WASD/arrow movement works, sound plays after the starting click, no red errors and no raylib `SHADER` compile failures in the browser console. If Chrome browser tools (claude-in-chrome MCP) are available, verify the console programmatically; otherwise ask Ben to eyeball it.
- [ ] `npx wrangler pages deploy` succeeds; the `*.pages.dev` URL loads and plays the same as local.
- [ ] Commits are small (scaffold / build script / deploy) with clear messages.

---
**On completion:** update `.handovers/handover_log.md` — set row 006 to ✅ done and fill the Completed date. If you could not finish, set 🔄 in-progress and append a note row describing what remains (e.g. "built locally, deploy blocked on wrangler auth").
