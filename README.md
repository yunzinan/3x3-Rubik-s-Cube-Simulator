# 3x3-Rubik-s-Cube-Simulator

Rubik's Cube Simulator, implemented in Modern C++.

Current Version only supports Desktop Application. Web Application is coming 
soon.

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




