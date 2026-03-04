# 3x3-Rubik-s-Cube-Simulator

Rubik's Cube Simulator, implemented in Modern C++.

## Quick Start

### Prerequisites

- **CMake** ≥ 3.20
- **C++17** compiler (e.g. GCC 9+, Clang 10+, MSVC 2019+)
- **SDL2** (required for the desktop app: windowing and input)
- **SDL2_mixer** (optional, for move sound effect on desktop; if not found, the app builds without sound)

Install SDL2 and SDL2_mixer (examples):

- **macOS (Homebrew):** `brew install sdl2 sdl2_mixer`
- **Ubuntu/Debian:** `sudo apt install libsdl2-dev libsdl2-mixer-dev`
- **Windows:** Download from [SDL](https://wiki.libsdl.org/Installation) or use vcpkg: `vcpkg install sdl2 sdl2-mixer`

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

### WebApp 配置全流程

Web 版使用 Emscripten 编译为 WebAssembly，与桌面版共用同一套渲染、动画与交互逻辑。按以下步骤可完成从环境到在浏览器中运行的全流程配置。

#### 1. 环境准备

- **系统要求**：与 Quick Start 一致（CMake ≥ 3.20、C++17 编译器）。Web 构建不需要安装 SDL2。
- **安装 Emscripten SDK (emsdk)**：
  - 打开 [Emscripten 官方下载页](https://emscripten.org/docs/getting_started/downloads.html)，按文档安装 emsdk。
  - 常见方式（在选定的安装目录下）：
    ```bash
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    ```
- **激活当前终端环境**（每次新开终端做 Web 构建前都要执行一次）：
  ```bash
  # 若 emsdk 在 ~/emsdk（请按实际路径修改）：
  source ~/emsdk/emsdk_env.sh
  # 验证：
  which emcmake   # 应输出路径，例如 .../emsdk/upstream/emscripten/emcmake
  which emcc      # 应输出 emcc 路径
  ```

#### 2. 创建 Web 构建目录并配置

在**项目根目录**下（且已执行过 `source .../emsdk_env.sh`）：

```bash
# 新建并进入 web 专用构建目录（不要复用桌面版的 build 目录）
mkdir build-web && cd build-web

# 使用 Emscripten 工具链配置，关闭桌面版、开启 Web 版
emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..

# 可选：Release 构建
# emcmake cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..
```

若之前在该目录用普通 CMake 配置过，需先清空再按上面步骤用 `emcmake` 重新配置：

```bash
cd /path/to/project
rm -rf build-web
mkdir build-web && cd build-web
emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..
```

#### 3. 编译

仍在 `build-web` 目录下：

```bash
cmake --build .
# 多核加速示例：cmake --build . -j$(nproc)
```

#### 4. 定位构建产物

编译完成后，产物可能在以下位置之一（取决于 CMake 生成器）：

- `build-web/Release/bin/`
- `build-web/src/`
- 或直接在 `build-web/` 下

需要的文件包括：

- `RubiksCubeWeb.html` — 入口页面
- `RubiksCubeWeb.js` — 胶水脚本
- `RubiksCubeWeb.wasm` — WebAssembly 模块
- `RubiksCubeWeb.data` — 预加载资源（含 `sequence.json`）

若不确定，可在 `build-web` 下执行：`find . -name "RubiksCubeWeb.html"` 找到 HTML 所在目录。

#### 5. 启动本地 HTTP 服务并在浏览器中打开

WASM 必须通过 HTTP(S) 加载，不能直接双击 `.html` 文件。在**产物所在目录**启动本地服务器：

```bash
# 进入产物目录（根据上一步结果二选一或按实际路径调整）
cd build-web/Release/bin
# 或：cd build-web/src

# 使用 Emscripten 自带的 HTTP 服务（需已 source emsdk_env.sh）
emrun --no_browser --port 8080 .
```

然后在浏览器中访问：

**http://localhost:8080/RubiksCubeWeb.html**

即可看到魔方界面。若使用其他端口，请将 `8080` 改为对应端口。

**可选：使用 Python 启动服务**

若未使用 `emrun`，可在产物目录下用 Python 启动简单 HTTP 服务：

```bash
cd build-web/Release/bin   # 或你的产物目录
python3 -m http.server 8080
# 然后访问 http://localhost:8080/RubiksCubeWeb.html
```

#### 6. 使用与注意事项

- **序列文件路径**：在 Web 版中加载/保存序列使用 Emscripten 虚拟文件系统路径，界面默认路径为 `/sequence.json`。
- **鼠标操作**：平移视角可使用右键/中键拖拽，或 `Shift + 左键拖拽`（与桌面版一致）。
- 终端中若出现 `GET /.well-known/...` 的 404，多为浏览器开发者工具自动请求，可忽略，不影响魔方页面。

---

### Build and run the WebAssembly app (English)

The web target is built with Emscripten and keeps the same rendering / animation / interaction logic as the desktop version.

**Prerequisite:** Install [Emscripten SDK (emsdk)](https://emscripten.org/docs/getting_started/downloads.html), then **activate it in the current terminal** so `emcmake` and `emcc` are on `PATH`:

```bash
source ~/emsdk/emsdk_env.sh
which emcmake
```

From the project root (with emsdk activated):

```bash
mkdir build-web && cd build-web
emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..
cmake --build .
```

Locate the output (e.g. `build-web/Release/bin/` or `build-web/src/`), then serve that directory over HTTP (e.g. `emrun --no_browser --port 8080 .`) and open **http://localhost:8080/RubiksCubeWeb.html** in the browser.

## Features

- **Move sound effect:** Each cube move (keyboard or loaded sequence) plays a short sound. Place `audio.mp3` in the project root; the desktop build copies it next to the executable, and the web build preloads it. Desktop requires SDL2_mixer for playback; if SDL2_mixer is not found, the app still builds and runs without sound.

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




