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
set(APP_SOURCES
    app/DesktopApp.h   app/DesktopApp.cpp
    app/ImGuiUI.h      app/ImGuiUI.cpp
)

# ── Desktop Application ─────────────────────────────────────────────────────

if(BUILD_DESKTOP_APP)
    add_executable(RubiksCube
        main.cpp
        ${CORE_SOURCES}
        ${RENDER_SOURCES}
        ${ANIMATION_SOURCES}
        ${UTILS_SOURCES}
        ${APP_SOURCES}
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
endif()
