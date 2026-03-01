include(FetchContent)

# Use master branches of Corrade/Magnum/MagnumIntegration instead of the
# outdated v2020.06 tags.  The old release has missing #include's that break
# on Apple Clang 17+.  Master is actively maintained and compatible.

# ── Corrade ──────────────────────────────────────────────────────────────────
FetchContent_Declare(corrade
    GIT_REPOSITORY git@github.com:mosra/corrade.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ── Magnum ───────────────────────────────────────────────────────────────────
set(WITH_SDL2APPLICATION ON  CACHE BOOL "" FORCE)
if(BUILD_WEB_APP)
    set(WITH_EMSCRIPTENAPPLICATION ON CACHE BOOL "" FORCE)
endif()
set(WITH_MESHTOOLS      ON  CACHE BOOL "" FORCE)
set(WITH_PRIMITIVES     ON  CACHE BOOL "" FORCE)
set(WITH_SCENEGRAPH     ON  CACHE BOOL "" FORCE)
set(WITH_SHADERS        ON  CACHE BOOL "" FORCE)
set(WITH_TRADE          ON  CACHE BOOL "" FORCE)
FetchContent_Declare(magnum
    GIT_REPOSITORY git@github.com:mosra/magnum.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ── Magnum Integration (Dear ImGui) ─────────────────────────────────────────
set(WITH_IMGUI ON CACHE BOOL "" FORCE)
FetchContent_Declare(magnum-integration
    GIT_REPOSITORY git@github.com:mosra/magnum-integration.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ── Dear ImGui (needed by magnum-integration) ───────────────────────────────
FetchContent_Declare(imgui
    GIT_REPOSITORY git@github.com:ocornut/imgui.git
    GIT_TAG        v1.90.4
    GIT_SHALLOW    TRUE
)

# ── spdlog ───────────────────────────────────────────────────────────────────
FetchContent_Declare(spdlog
    GIT_REPOSITORY git@github.com:gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)

# ── nlohmann/json (for File I/O) ────────────────────────────────────────────
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(json
    GIT_REPOSITORY git@github.com:nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# Make all dependencies available.
# Corrade must be available before Magnum; Magnum before magnum-integration.
FetchContent_MakeAvailable(corrade)
FetchContent_MakeAvailable(magnum)

# Fix Emscripten requestAnimationFrame callback signature mismatch: browser passes
# (double timestamp) but Magnum expected (void*). Patch to (void*, double) and
# pass [state, timestamp] from JS so dynCall signature matches.
if(BUILD_WEB_APP AND magnum_SOURCE_DIR)
  set(_mag_h "${magnum_SOURCE_DIR}/src/Magnum/Platform/EmscriptenApplication.h")
  set(_mag_cpp "${magnum_SOURCE_DIR}/src/Magnum/Platform/EmscriptenApplication.cpp")
  set(_mag_js "${magnum_SOURCE_DIR}/src/Magnum/Platform/Platform.js.in")
  file(READ "${_mag_h}" _mag_h_content)
  string(REPLACE "int (*_callback)(void*);" "int (*_callback)(void*, double);" _mag_h_content "${_mag_h_content}")
  file(WRITE "${_mag_h}" "${_mag_h_content}")
  file(READ "${_mag_cpp}" _mag_cpp_content)
  string(REPLACE "void magnumPlatformRequestAnimationFrame(int(*callback)(void*), void* state);" "void magnumPlatformRequestAnimationFrame(int(*callback)(void*, double), void* state);" _mag_cpp_content "${_mag_cpp_content}")
  string(REPLACE "[](void* userData) -> int {" "[](void* userData, double) -> int {" _mag_cpp_content "${_mag_cpp_content}")
  file(WRITE "${_mag_cpp}" "${_mag_cpp_content}")
  file(READ "${_mag_js}" _mag_js_content)
  string(REPLACE "var drawEvent = function() {" "var drawEvent = function(timestamp) {" _mag_js_content "${_mag_js_content}")
  string(REPLACE "if(!dynCall('ii', callback, [state]))" "if(!dynCall('iid', callback, [state, timestamp || 0]))" _mag_js_content "${_mag_js_content}")
  string(REPLACE "if(!dynCall('id', callback, [state, timestamp || 0]))" "if(!dynCall('iid', callback, [state, timestamp || 0]))" _mag_js_content "${_mag_js_content}")
  file(WRITE "${_mag_js}" "${_mag_js_content}")
endif()

# ImGui must be available before magnum-integration (it needs IMGUI_DIR).
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_MakeAvailable(magnum-integration)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(json)
