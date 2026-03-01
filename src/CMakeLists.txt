# ── Collect sources by module ────────────────────────────────────────────────

# Core
set(CORE_SOURCES
    core/Move.h        core/Move.cpp
    core/CubeState.h   core/CubeState.cpp
    core/History.h     core/History.cpp
)

# Render
set(RENDER_SOURCES
    render/CubieDrawable.h   render/CubieDrawable.cpp
    render/CubeScene.h       render/CubeScene.cpp
    render/ArcBallCamera.h   render/ArcBallCamera.cpp
)

# Animation
set(ANIMATION_SOURCES
    animation/AnimationManager.h   animation/AnimationManager.cpp
)

# Utils
set(UTILS_SOURCES
    utils/Logger.h     utils/Logger.cpp
    utils/FileIO.h     utils/FileIO.cpp
)

# App
set(APP_COMMON_SOURCES
    app/ImGuiUI.h      app/ImGuiUI.cpp
)

set(DESKTOP_APP_SOURCES
    app/DesktopApp.h   app/DesktopApp.cpp
)

set(WEB_APP_SOURCES
    app/WebApp.h       app/WebApp.cpp
)

# ── Desktop Application ─────────────────────────────────────────────────────

if(BUILD_DESKTOP_APP)
    add_executable(RubiksCube
        main.cpp
        ${CORE_SOURCES}
        ${RENDER_SOURCES}
        ${ANIMATION_SOURCES}
        ${UTILS_SOURCES}
        ${APP_COMMON_SOURCES}
        ${DESKTOP_APP_SOURCES}
    )

    target_include_directories(RubiksCube PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(RubiksCube PRIVATE
        Corrade::Utility
        Magnum::Sdl2Application
        Magnum::GL
        Magnum::Magnum
        Magnum::MeshTools
        Magnum::Primitives
        Magnum::SceneGraph
        Magnum::Shaders
        Magnum::Trade
        MagnumIntegration::ImGui
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )

    # Copy ImGui TTF fonts next to executable for HiDPI crisp text
    add_custom_command(TARGET RubiksCube POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:RubiksCube>/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${IMGUI_DIR}/misc/fonts" "$<TARGET_FILE_DIR:RubiksCube>/fonts"
        COMMENT "Copying ImGui fonts for HiDPI"
    )
endif()

# ── Web Application (Emscripten + WebAssembly) ──────────────────────────────

if(BUILD_WEB_APP)
    set(_web_emscripten OFF)
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(_web_emscripten ON)
    endif()
    if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten\\.cmake$")
        set(_web_emscripten ON)
    endif()
    if(NOT _web_emscripten)
        message(FATAL_ERROR "BUILD_WEB_APP=ON requires Emscripten toolchain (emcmake/em++).")
    endif()
    unset(_web_emscripten)

    add_executable(RubiksCubeWeb
        main_web.cpp
        ${CORE_SOURCES}
        ${RENDER_SOURCES}
        ${ANIMATION_SOURCES}
        ${UTILS_SOURCES}
        ${APP_COMMON_SOURCES}
        ${WEB_APP_SOURCES}
    )

    target_include_directories(RubiksCubeWeb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Silence deprecation warnings from Magnum EmscriptenApplication (uses old __EMSCRIPTEN_* macros)
    target_compile_options(RubiksCubeWeb PRIVATE -Wno-deprecated-pragma)

    target_link_libraries(RubiksCubeWeb PRIVATE
        Corrade::Utility
        Magnum::EmscriptenApplication
        Magnum::GL
        Magnum::Magnum
        Magnum::MeshTools
        Magnum::Primitives
        Magnum::SceneGraph
        Magnum::Shaders
        Magnum::Trade
        MagnumIntegration::ImGui
        spdlog::spdlog
        nlohmann_json::nlohmann_json
    )

    target_link_options(RubiksCubeWeb PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s USE_WEBGL2=0"
        "SHELL:-s FULL_ES2=1"
        "SHELL:-s MIN_WEBGL_VERSION=1"
        "SHELL:-s MAX_WEBGL_VERSION=1"
        "SHELL:-s FORCE_FILESYSTEM=1"
        "SHELL:-s EMULATE_FUNCTION_POINTER_CASTS=1"
        "SHELL:-s RUNTIME_DEBUG=1"
        "SHELL:-s ASSERTIONS=2"
        "SHELL:--profiling"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/sequence.json@/sequence.json"
        "SHELL:--preload-file ${IMGUI_DIR}/misc/fonts@/fonts"
        "SHELL:--pre-js ${CMAKE_SOURCE_DIR}/web/pre.js"
    )

    set_target_properties(RubiksCubeWeb PROPERTIES SUFFIX ".html")
endif()
