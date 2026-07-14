#!/usr/bin/env bash
# Builds the claude/ competitor game to WebAssembly for Cloudflare Pages.
# The judged sources are never modified: claude/src is copied into the
# gitignored web/build/ and only the copy is patched (shader #version lines).
# Run from anywhere: the script cd's to the repo root itself.
set -euo pipefail

cd "$(dirname "$0")/.."

RAYLIB_TAG="6.0"
RAYLIB_SRC="web/build/raylib"
RAYLIB_BUILD="web/build/raylib-build"
SRC_WEB="web/build/src-web"
DIST="web/dist"

command -v em++ >/dev/null 2>&1 || {
  echo "em++ not found — install with: brew install emscripten" >&2
  exit 1
}

# raylib for web, OpenGL ES 3 — required so the '#version 300 es' shaders
# produced by the transform below actually compile under WebGL 2.
if [ ! -f "$RAYLIB_BUILD/raylib/libraylib.a" ]; then
  if [ ! -d "$RAYLIB_SRC" ]; then
    git clone --depth 1 --branch "$RAYLIB_TAG" https://github.com/raysan5/raylib.git "$RAYLIB_SRC"
  fi
  emcmake cmake -S "$RAYLIB_SRC" -B "$RAYLIB_BUILD" \
    -DPLATFORM=Web -DGRAPHICS=GRAPHICS_API_OPENGL_ES3 \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF
  cmake --build "$RAYLIB_BUILD" -j
fi

# Copy sources; convert the three embedded desktop-GLSL (330) shaders to
# GLSL ES 300. highp is mandatory in ES 3.00 fragment shaders and safe in
# the vertex stage (mediump there could jitter gl_Position math).
rm -rf "$SRC_WEB"
cp -R claude/src "$SRC_WEB"
# tmp-file dance instead of sed -i: BSD and GNU sed disagree on -i syntax,
# and this script now also runs on Linux in CI.
sed 's/#version 330/#version 300 es\'$'\nprecision highp float;/' "$SRC_WEB/render.cpp" \
  > "$SRC_WEB/render.cpp.tmp" && mv "$SRC_WEB/render.cpp.tmp" "$SRC_WEB/render.cpp"
[ "$(grep -c '300 es' "$SRC_WEB/render.cpp")" -eq 3 ] || {
  echo "shader transform failed: expected 3 '#version 300 es' occurrences" >&2
  exit 1
}

# -sASYNCIFY lets the game's unmodified blocking main loop yield to the
# browser; --preload-file mounts the frozen assets/ at virtual /assets,
# which the game's '../assets/' probe resolves to from CWD '/'.
# -sGROWABLE_ARRAYBUFFERS=0: with memory growth, emscripten 6 otherwise backs
# the heap with a resizable ArrayBuffer, and Chrome rejects WebGL uploads from
# views over resizable buffers ("texImage2D: ... must not be resizable") —
# the first texture upload throws and the canvas stays black.
mkdir -p "$DIST"
em++ -std=c++17 -O2 "$SRC_WEB"/*.cpp \
  -I "$RAYLIB_SRC/src" \
  "$RAYLIB_BUILD/raylib/libraylib.a" \
  -sUSE_GLFW=3 \
  -sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 \
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sGROWABLE_ARRAYBUFFERS=0 \
  --preload-file assets@assets \
  -o "$DIST/game.js"
cp web/index.html "$DIST/index.html"

echo "Build complete:"
ls -lh "$DIST"
