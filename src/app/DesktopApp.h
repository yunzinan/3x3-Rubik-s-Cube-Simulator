#pragma once

#include "animation/AnimationManager.h"
#include "app/ImGuiUI.h"
#include "audio/AudioPlayer.h"
#include "core/CubeState.h"
#include "core/History.h"
#include "render/CubeScene.h"

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/ImGuiIntegration/Context.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Timeline.h>

namespace rubik {

class DesktopApp : public Magnum::Platform::Sdl2Application {
public:
    explicit DesktopApp(const Arguments& arguments);

private:
    enum class DragButton {
        None,
        Left,
        Right,
        Middle
    };

    void drawEvent() override;
    void viewportEvent(ViewportEvent& event) override;
    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;
    void pointerPressEvent(PointerEvent& event) override;
    void pointerReleaseEvent(PointerEvent& event) override;
    void pointerMoveEvent(PointerMoveEvent& event) override;
    void scrollEvent(ScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;

    void tryEnqueueMove(Move move);
    void doUndo();
    void doRedo();

    CubeState cubeState_;
    History   history_;

    AnimationManager animManager_;
    AudioPlayer      audioPlayer_;
    CubeScene        cubeScene_;
    ImGuiUI          ui_;

    Magnum::ImGuiIntegration::Context imgui_{Magnum::NoCreate};
    Magnum::Timeline timeline_;

    DragButton dragButton_ = DragButton::None;
    Magnum::Vector2i lastMousePos_;
    bool ctrlHeld_ = false;
};

} // namespace rubik
