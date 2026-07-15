#!/usr/bin/env bash
# Builds the claude/ game to WebAssembly for Cloudflare Pages.
# Run from anywhere: the script cd's to the repo root itself.
set -euo pipefail

cd "$(dirname "$0")/.."

RAYLIB_TAG="6.0"
RAYLIB_SRC="web/build/raylib"
RAYLIB_BUILD="web/build/raylib-build"
DIST="web/dist"

command -v em++ >/dev/null 2>&1 || {
  echo "em++ not found — install with: brew install emscripten" >&2
  exit 1
}

# raylib for web, OpenGL ES 3 — required so the game's '#version 300 es'
# shader variant (GLSL_HEADER in claude/src/render.cpp) compiles under WebGL 2.
if [ ! -f "$RAYLIB_BUILD/raylib/libraylib.a" ]; then
  if [ ! -d "$RAYLIB_SRC" ]; then
    git clone --depth 1 --branch "$RAYLIB_TAG" https://github.com/raysan5/raylib.git "$RAYLIB_SRC"
  fi
  emcmake cmake -S "$RAYLIB_SRC" -B "$RAYLIB_BUILD" \
    -DPLATFORM=Web -DGRAPHICS=GRAPHICS_API_OPENGL_ES3 \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
  cmake --build "$RAYLIB_BUILD" -j
fi

# -sASYNCIFY lets the game's unmodified blocking main loop yield to the
# browser; --preload-file mounts assets/ at virtual /assets, which the
# game's '../assets/' probe resolves to from CWD '/'.
# -sGROWABLE_ARRAYBUFFERS=0: with memory growth, emscripten 6 otherwise backs
# the heap with a resizable ArrayBuffer, and Chrome rejects WebGL uploads from
# views over resizable buffers ("texImage2D: ... must not be resizable") —
# the first texture upload throws and the canvas stays black.
mkdir -p "$DIST"
em++ -std=c++17 -O2 claude/src/*.cpp \
  -I "$RAYLIB_SRC/src" \
  "$RAYLIB_BUILD/raylib/libraylib.a" \
  -sUSE_GLFW=3 \
  -sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 \
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sGROWABLE_ARRAYBUFFERS=0 \
  -sEXPORTED_RUNTIME_METHODS=stringToUTF8 \
  --preload-file assets@assets \
  -o "$DIST/game.js"
# Version-stamp the shell page: index.html's BUILD appends ?v= to every asset
# URL, so Safari's stubbornly cached game.js/.wasm/.data get refetched per build.
# A dirty tree gets a timestamp too — consecutive dirty builds would otherwise
# share one id and Safari would serve the previous build against a phone again.
# RELAY_URL (optional env, e.g. wss://mp.example.com) wires the arena's relay
# into the page; left unset, the page only reaches a relay on localhost/LAN.
BUILD_ID="$(git describe --always --dirty 2>/dev/null || echo dev)"
case "$BUILD_ID" in
  *-dirty|dev) BUILD_ID="$BUILD_ID.$(date +%s)" ;;
esac
sed -e "s/__BUILD_ID__/$BUILD_ID/g" \
    -e "s|__RELAY_URL__|${RELAY_URL:-}|g" web/index.html > "$DIST/index.html"

echo "Build complete:"
ls -lh "$DIST"
