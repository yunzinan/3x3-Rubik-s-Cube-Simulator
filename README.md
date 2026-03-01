# 3x3-Rubik-s-Cube-Simulator

Rubik's Cube Simulator, implemented in Modern C++.

Current Version only supports Desktop Application. Web Application is coming 
soon.

## Quick Start

#TODO

## Features

User Interface:

- [ ] Camera Navigation: Rotate View(Orbit) / Zoom In & Out / Pan View
- [ ] User Control: Keyboard & Mouse control & File I/O, see [interface-specification.md](interface-specification.md) for more details.
- [ ] 



Visualization:
- [ ] 3D Cube Visualization
- [ ] Camera Navigation: change viewpoint of the cube
- [ ] Move Animation: perform a single move of the cube


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




