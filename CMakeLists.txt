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

# Validate toolchain early, before fetching dependencies.
# Accept when either system is Emscripten or the Emscripten toolchain file is
# in use (avoids false failure when re-running emcmake in a dir first configured with native cmake).
set(_using_emscripten OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(_using_emscripten ON)
endif()
if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten\\.cmake$")
    set(_using_emscripten ON)
endif()
if(BUILD_WEB_APP AND NOT _using_emscripten)
    message(FATAL_ERROR
        "BUILD_WEB_APP=ON requires Emscripten toolchain.\n"
        "Configure with: emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON ..\n"
        "If this directory was previously configured without Emscripten, run from a clean build dir:\n"
        "  rm -rf build-web && mkdir build-web && cd build-web\n"
        "  emcmake cmake -DBUILD_DESKTOP_APP=OFF -DBUILD_WEB_APP=ON .."
    )
endif()
unset(_using_emscripten)

# ----- Dependencies -----
include(cmake/FetchDependencies.cmake)

# ----- Source -----
add_subdirectory(src)
