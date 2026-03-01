# 3x3-Rubik-s-Cube-Simulator

Rubik's Cube Simulator, implemented in Modern C++.

## Quick Start

### Prerequisites

- **CMake** ≥ 3.20
- **C++17** compiler (e.g. GCC 9+, Clang 10+, MSVC 2019+)
- **SDL2** (required for the desktop app: windowing and input)

Install SDL2 (examples):

- **macOS (Homebrew):** `brew install sdl2`
- **Ubuntu/Debian:** `sudo apt install libsdl2-dev`
- **Windows:** Download from [SDL](https://wiki.libsdl.org/Installation) or use vcpkg: `vcpkg install sdl2`

All other dependencies (Corrade, Magnum, MagnumIntegration, Dear ImGui, spdlog, nlohmann/json) are fetched and configured by CMake via FetchContent; no manual install is needed.

### Configure and build

From the project root:

```bash
# Create and enter build directory
mkdir build && cd build

# Configure (builds the desktop RubiksCube app by default)
cmake ..

# Build
cmake --build .
```

Optional CMake options:

- `-DBUILD_DESKTOP_APP=ON` — build the SDL2 desktop application (default: ON)
- `-DBUILD_WEB_APP=OFF` — do not build the Emscripten web app (default: OFF)
- `-DBUILD_TESTS=OFF` — do not build unit tests (default: OFF)

Example for a Release build with parallel jobs:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### Run the application

After building, from the `build` directory run:

```bash
./RubiksCube
```

If the executable is under `build/src` (depends on your generator), use:

```bash
./src/RubiksCube
```

On first run, ImGui fonts are copied to a `fonts/` directory next to the executable for HiDPI; if the UI text looks wrong, ensure that directory exists.

### Build and run the WebAssembly app

The web target is built with Emscripten and keeps the same rendering / animation / interaction logic as the desktop version.

**Prerequisite:** Install [Emscripten SDK (emsdk)](https://emscripten.org/docs/getting_started/downloads.html), then **activate it in the current terminal** so `emcmake` and `emcc` are on `PATH`:

```bash
# If emsdk is at ~/emsdk (adjust path if needed):
source ~/emsdk/emsdk_env.sh
# Verify:
which emcmake   # should print a path, e.g. .../emsdk/upstream/emscripten/emcmake
```

From the project root (with emsdk activated):

```bash
# Create and enter web build directory
mkdir build-web && cd build-web

# Configure with Emscripten toolchain
emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..

# Build
cmake --build .
```

Build output (e.g. `build-web/Release/bin/` or `build-web/src/`) contains:

- `RubiksCubeWeb.html`
- `RubiksCubeWeb.js`
- `RubiksCubeWeb.wasm`
- `RubiksCubeWeb.data` (contains preloaded `sequence.json`)

**在浏览器中打开界面：** 构建完成后不能直接双击 `.html` 文件（WASM 需通过 HTTP 加载）。需要在本机起一个本地服务器，再在浏览器访问：

```bash
# 进入产物所在目录（根据你的 CMake 生成器可能是 Release/bin 或就在 build-web 下）
cd build-web/Release/bin
# 若没有 Release/bin，先看看 build-web 下是否有 RubiksCubeWeb.html，再 cd 到该目录

# 用 Emscripten 自带的服务器（需已 source emsdk_env.sh）
emrun --no_browser --port 8080 .
```

然后在浏览器打开：**http://localhost:8080/RubiksCubeWeb.html** 即可看到魔方界面。页面使用 Emscripten 默认壳，可能带有一块控制/输出区域，魔方 3D 视图为主要区域。

Notes:

- In browser, loading/saving sequence uses Emscripten virtual filesystem paths (default path in UI: `/sequence.json`).
- For mouse pan, use right/middle drag or `Shift + Left Drag` (same as desktop fallback behavior).
- 若终端里出现 `GET /.well-known/...` 的 404，是 Chrome 开发者工具自动请求，可忽略，不影响魔方页面。

## Features

User Interface:

- [x] Camera Navigation: Rotate View(Orbit) / Zoom In & Out / Pan View
- [x] User Control: Keyboard & Mouse control & File I/O, see [interface-specification.md](interface-specification.md) for more details.



Visualization:
- [x] 3D Cube Visualization
- [x] Camera Navigation: change viewpoint of the cube
- [x] Move Animation: perform a single move of the cube


## Requirements

- [Magnum](https://github.com/mosra/magnum) - A modern C++11/14 graphics engine for games and data visualization.
- [Corrade](https://github.com/mosra/corrade) - A library for C++11/14/17/20 providing containers, algorithms and utilities to help with programming in modern C++.
- SDL2 - A cross-platform development library for creating graphical user interfaces and games.
- Emscripten - A compiler toolchain for compiling to WebAssembly.
- Dear ImGui - A library for creating graphical user interfaces and widgets.
- [spdlog](https://github.com/gabime/spdlog) - A fast C++ logging library.
- CMake - A build system generator.

## Architecture

### Utils

- `Math`: mathematical utilities.
- `Timer`: timer utilities.
- `Logger`: logging utilities.
- `Config`: configuration utilities.
- `File`: file utilities.

### Core

- `CubeState`: state of the cube
- `Move`: move of the cube
- `History`: history of moves performed.

### Render

- `Renderer`: renderer for the cube.
- `Camera`: camera for the cube.
- `Light`: light for the cube.
- `Material`: material for the cube.
- `Mesh`: mesh for the cube.
- `Texture`: texture for the cube.

Render System: Scene Graph of Magnum.

- Root

    - CubeRoot

        - Cubie[27] Nodes

### App

- `DesktopApp`: SDL2 based desktop application.
- `WebApp`: WebAssembly based web application.
- `UI`: Dear ImGui based user interface.




