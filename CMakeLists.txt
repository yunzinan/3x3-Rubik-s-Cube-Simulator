cmake_minimum_required(VERSION 3.20)

# Corrade v2020.06 requests CMake < 3.5; modern CMake removed that. Tell CMake
# to use at least 3.5 when configuring fetched dependencies.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

project(RubiksCubeSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----- Options -----
option(BUILD_DESKTOP_APP "Build the SDL2 desktop application" ON)
option(BUILD_WEB_APP    "Build the Emscripten web application" OFF)
option(BUILD_TESTS      "Build unit tests" OFF)

# ----- Dependencies -----
include(cmake/FetchDependencies.cmake)

# ----- Source -----
add_subdirectory(src)
