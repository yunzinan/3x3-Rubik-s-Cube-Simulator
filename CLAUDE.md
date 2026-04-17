# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

All dependencies (Corrade, Magnum, Dear ImGui, spdlog, nlohmann/json) are auto-fetched via CMake FetchContent. Only SDL2/SDL2_mixer (desktop) and Emscripten (web) must be installed manually.

### Desktop App
```bash
mkdir build && cd build
cmake ..
cmake --build .
./src/RubiksCube          # or ./RubiksCube depending on platform
```

### Web App
```bash
source ~/emsdk/emsdk_env.sh   # must be run each session
mkdir build-web && cd build-web
emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..
cmake --build .
cd bin && emrun --no_browser --port 8080 .
# Open http://localhost:8080/RubiksCubeWeb.html
```

### CMake Options
- `-DBUILD_DESKTOP_APP=ON/OFF` (default ON)
- `-DBUILD_WEB_APP=ON/OFF` (default OFF) — requires Emscripten toolchain; use a separate `build-web/` directory
- `-DBUILD_TESTS=ON/OFF` (default OFF)

The desktop and web builds **must** use separate build directories. Running `cmake ..` in a directory previously configured with `emcmake` (or vice versa) will fail — delete the directory and start fresh.

## Architecture

All source lives under `src/` in the `rubik` namespace. There are two entry points: `main.cpp` (desktop) and `main_web.cpp` (web), each instantiating the corresponding App class.

### Module Overview

**`core/`** — pure logic, no rendering
- `Move` — represents a single 90° face turn: `Face` (F/B/U/D/L/R) × `Direction` (CW/CCW). Serializes to/from single chars (`'f'` = F CW, `'F'` = F CCW).
- `CubeState` — 26 `CubieInfo` structs (excludes hidden center). Each cubie tracks its current grid position (`Vector3i` in {-1,0,1}³) and accumulated rotation matrix. `applyMove()` updates these.
- `History` — undo/redo stack wrapping `CubeState`.

**`animation/`**
- `AnimationManager` — FSM with states `Idle`/`Moving`. Maintains a move queue. Calls `onMoveBegin` / `onMoveComplete` callbacks at the right moments. `update(dt)` must be called every frame.

**`render/`**
- `CubieDrawable` — one Magnum `SceneGraph::Drawable3D` per cubie; renders with Phong shading.
- `CubeScene` — owns the full Magnum scene graph (26 cubie `Object3D` nodes + a `rotationGroup_` node used during animations). Key flow: `beginFaceRotation()` → `setFaceRotationAngle()` each frame → `endFaceRotation()` (which calls `syncFromState()`).
- `ArcBallCamera` — orbit/zoom/pan camera.

**`app/`**
- `DesktopApp` — subclasses `Magnum::Platform::Sdl2Application`; wires together all modules, handles keyboard/mouse events.
- `WebApp` — subclasses `Magnum::Platform::EmscriptenApplication`; same wiring for WASM target.
- `ImGuiUI` — Dear ImGui overlay: move buttons, undo/redo, file load/save, auto-play controls.

**`audio/`** — `AudioPlayer` wraps SDL2_mixer (desktop) / Web Audio (web). Optional: only active if `audio.mp3` is present in the project root.

**`utils/`** — `Logger` (spdlog wrapper), `FileIO` (nlohmann/json, reads/writes `sequence.json` format).

### Animation Flow

The move pipeline: user input or file load → `AnimationManager::enqueueMove()` → `onMoveBegin` fires → `CubeScene::beginFaceRotation()` → per-frame `setFaceRotationAngle()` → `onMoveComplete` fires → `CubeState::applyMove()` → `CubeScene::endFaceRotation()` / `syncFromState()`.

During animation, new move inputs are blocked. Camera controls are always active (independent of animation state).

### Sequence File Format

```json
{ "control_sequence": ["f", "F", "b", "B", "u", "U", "d", "D", "l", "r"] }
```

Lowercase = CW, uppercase = CCW. `sequence.json` in the project root is preloaded into the WASM binary at build time.

## Key Conventions

- Grid coordinates: F=+Z, B=−Z, U=+Y, D=−Y, L=−X, R=+X.
- 26 cubies indexed 0–25; index layout is established in `CubeState::initCubies()`.
- No 180° moves — two consecutive 90° turns are used instead.
- The `rotationGroup_` node in `CubeScene` is a temporary parent used only during a face-turn animation; cubies are re-parented back to `cubeRoot_` when the animation ends.
