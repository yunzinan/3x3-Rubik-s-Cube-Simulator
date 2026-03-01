include(FetchContent)

# Use master branches of Corrade/Magnum/MagnumIntegration instead of the
# outdated v2020.06 tags.  The old release has missing #include's that break
# on Apple Clang 17+.  Master is actively maintained and compatible.

# ── Corrade ──────────────────────────────────────────────────────────────────
FetchContent_Declare(corrade
    GIT_REPOSITORY https://github.com/mosra/corrade.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ── Magnum ───────────────────────────────────────────────────────────────────
set(WITH_SDL2APPLICATION ON  CACHE BOOL "" FORCE)
set(WITH_MESHTOOLS      ON  CACHE BOOL "" FORCE)
set(WITH_PRIMITIVES     ON  CACHE BOOL "" FORCE)
set(WITH_SCENEGRAPH     ON  CACHE BOOL "" FORCE)
set(WITH_SHADERS        ON  CACHE BOOL "" FORCE)
set(WITH_TRADE          ON  CACHE BOOL "" FORCE)
FetchContent_Declare(magnum
    GIT_REPOSITORY https://github.com/mosra/magnum.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ── Magnum Integration (Dear ImGui) ─────────────────────────────────────────
set(WITH_IMGUI ON CACHE BOOL "" FORCE)
FetchContent_Declare(magnum-integration
    GIT_REPOSITORY https://github.com/mosra/magnum-integration.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# ── Dear ImGui (needed by magnum-integration) ───────────────────────────────
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
    GIT_SHALLOW    TRUE
)

# ── spdlog ───────────────────────────────────────────────────────────────────
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)

# ── nlohmann/json (for File I/O) ────────────────────────────────────────────
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# Make all dependencies available.
# Corrade must be available before Magnum; Magnum before magnum-integration.
FetchContent_MakeAvailable(corrade)
FetchContent_MakeAvailable(magnum)

# ImGui must be available before magnum-integration (it needs IMGUI_DIR).
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_MakeAvailable(magnum-integration)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(json)
