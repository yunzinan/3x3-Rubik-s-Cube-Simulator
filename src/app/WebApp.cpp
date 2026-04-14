#include "app/WebApp.h"
#include "utils/FileIO.h"
#include "utils/Logger.h"

#include <algorithm>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <imgui.h>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

WebApp::WebApp(const Arguments& arguments)
    : EmscriptenApplication{arguments, Configuration{}
          .setTitle("Rubik's Cube Simulator (Web)")
          .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    Logger::init();
    LOG_INFO("Starting Rubik's Cube Simulator (Web)");

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setClearColor(0xc8c8c8_rgbf);

    ImGui::CreateContext();
    const Vector2 uiSize = Vector2{windowSize()}/dpiScaling();
    const float fontScale = Float(framebufferSize().x()) /
        std::max(Float(uiSize.x()), 1.0f);
    const float fontPixels = (18.0f * fontScale);
    ImFont* font = nullptr;
#ifdef __EMSCRIPTEN__
    {
        const char* ttfPath = "/fonts/DroidSans.ttf";
        font = ImGui::GetIO().Fonts->AddFontFromFileTTF(ttfPath, fontPixels);
    }
#endif
    if (!font) {
        ImFontConfig fontCfg;
        fontCfg.SizePixels = fontPixels;
        ImGui::GetIO().Fonts->AddFontDefault(&fontCfg);
    }
    ImGui::GetStyle().ScaleAllSizes(fontScale);

    imgui_ = ImGuiIntegration::Context(*ImGui::GetCurrentContext(),
        uiSize, windowSize(), framebufferSize());

    cubeScene_.setup(framebufferSize());
    cubeScene_.syncFromState(cubeState_);

    audioPlayer_.init("/audio.mp3");  // preloaded to VFS

    animManager_.onMoveBegin = [this](Move m) {
        cubeScene_.beginFaceRotation(m.face, cubeState_);
        audioPlayer_.playMoveSound();
        LOG_DEBUG("Animation begin: {}", m.toChar());
    };

    animManager_.onMoveComplete = [this](Move m) {
        cubeState_.applyMove(m);
        cubeScene_.endFaceRotation(cubeState_);
        LOG_DEBUG("Animation complete: {}", m.toChar());
    };

    ui_.onReset = [this]() {
        animManager_.reset();
        cubeState_.reset();
        history_.clear();
        pendingMoves_.clear();
        pendingIndex_ = 0;
        cubeScene_.syncFromState(cubeState_);
        LOG_INFO("Cube reset");
    };

    ui_.onResetView = [this]() {
        cubeScene_.resetView();
        LOG_INFO("View reset");
    };

    ui_.onUndo = [this]() { doUndo(); };
    ui_.onRedo = [this]() { doRedo(); };
    ui_.setFilePathDefault("/sequence.json");

    ui_.onLoadFile = [this](const std::string& path) {
        auto moves = FileIO::loadControlSequence(path);
        if (!moves.empty()) {
            pendingMoves_ = std::move(moves);
            pendingIndex_ = 0;
            LOG_INFO("Loaded sequence: {} moves (use Auto-Play or Next Step to execute)",
                     pendingMoves_.size());
        }
    };

    ui_.onAutoPlay = [this]() { doAutoPlay(); };
    ui_.onGoNext   = [this]() { doGoNext(); };
    ui_.onGoBack   = [this]() { doGoBack(); };

    ui_.onSaveFile = [this](const std::string& path) {
        std::vector<Move> executed(history_.moves().begin(),
                                   history_.moves().begin() + history_.cursor());
        FileIO::saveControlSequence(path, executed);
    };

    ui_.onClearSequence = [this]() {
        animManager_.reset();
        history_.clear();
        pendingMoves_.clear();
        pendingIndex_ = 0;
        LOG_INFO("Sequence cleared");
    };

    timeline_.start();
}

void WebApp::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                                  GL::FramebufferClear::Depth);

    const float dt = timeline_.previousFrameDuration();
    animManager_.moveDuration = ui_.animationSpeed();
    animManager_.update(dt);

    if (auto m = animManager_.currentMove()) {
        cubeScene_.setFaceRotationAngle(m->face, m->direction,
                                         animManager_.progress());
    }

    cubeScene_.draw();

    imgui_.newFrame();
    ui_.draw(history_, animManager_.isAnimating(), animManager_.moveDuration,
             pendingIndex_, static_cast<int>(pendingMoves_.size()));
    imgui_.updateApplicationCursor(*this);
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
                                   GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    imgui_.drawFrame();
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    swapBuffers();
    timeline_.nextFrame();
    redraw();
}

void WebApp::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    cubeScene_.reshape(event.framebufferSize());
    imgui_.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
                    event.windowSize(), event.framebufferSize());
}

void WebApp::keyPressEvent(KeyEvent& event) {
    audioPlayer_.notifyUserGesture();  // Unlock web audio on first key (browser policy)
    if (imgui_.handleKeyPressEvent(event)) return;

    const bool ctrlHeld = bool(event.modifiers() & Modifier::Ctrl);
    const bool commandHeld = bool(event.modifiers() & Modifier::Super);
    const bool shortcutHeld = ctrlHeld || commandHeld;
    const bool shift = bool(event.modifiers() & Modifier::Shift);

    if (shortcutHeld) {
        if (event.key() == Key::Z) { doUndo(); event.setAccepted(); return; }
        if (event.key() == Key::Y) { doRedo(); event.setAccepted(); return; }
    }

    if (event.key() == Key::Equal) {
        cubeScene_.arcBall().zoom(1.0f);
        event.setAccepted();
        return;
    }
    if (event.key() == Key::Minus) {
        cubeScene_.arcBall().zoom(-1.0f);
        event.setAccepted();
        return;
    }

    auto charForKey = [&]() -> char {
        switch (event.key()) {
            case Key::F: return shift ? 'F' : 'f';
            case Key::B: return shift ? 'B' : 'b';
            case Key::U: return shift ? 'U' : 'u';
            case Key::D: return shift ? 'D' : 'd';
            case Key::L: return shift ? 'L' : 'l';
            case Key::R: return shift ? 'R' : 'r';
            default: return 0;
        }
    };

    if (const char c = charForKey()) {
        if (auto m = Move::fromChar(c)) {
            tryEnqueueMove(*m);
            event.setAccepted();
        }
    }
}

void WebApp::keyReleaseEvent(KeyEvent& event) {
    imgui_.handleKeyReleaseEvent(event);
}

void WebApp::pointerPressEvent(PointerEvent& event) {
    audioPlayer_.notifyUserGesture();  // Unlock web audio on first click (browser policy)
    imgui_.handlePointerPressEvent(event);
    // WantCaptureMouse is from previous frame; use current position to avoid UI clicks rotating the cube
    if (ImGui::GetIO().WantCaptureMouse || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
        return;

    if (event.pointer() == Pointer::MouseRight ||
        event.pointer() == Pointer::MouseMiddle) {
        dragButton_ = event.pointer() == Pointer::MouseRight ?
            DragButton::Right : DragButton::Middle;
        lastMousePos_ = Vector2i{event.position()};
        event.setAccepted();
        return;
    }

    if (event.pointer() == Pointer::MouseLeft) {
        dragButton_ = DragButton::Left;
        lastMousePos_ = Vector2i{event.position()};
        event.setAccepted();
    }
}

void WebApp::pointerReleaseEvent(PointerEvent& event) {
    imgui_.handlePointerReleaseEvent(event);
    if (ImGui::GetIO().WantCaptureMouse ||
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
        dragButton_ = DragButton::None;  // clear so next move does not rotate
        return;
    }

    const bool releasingActiveButton =
        (event.pointer() == Pointer::MouseLeft  && dragButton_ == DragButton::Left) ||
        (event.pointer() == Pointer::MouseRight && dragButton_ == DragButton::Right) ||
        (event.pointer() == Pointer::MouseMiddle && dragButton_ == DragButton::Middle);
    if (!releasingActiveButton) return;

    dragButton_ = DragButton::None;
    event.setAccepted();
}

void WebApp::pointerMoveEvent(PointerMoveEvent& event) {
    imgui_.handlePointerMoveEvent(event);
    if (ImGui::GetIO().WantCaptureMouse ||
        ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        return;

    if (dragButton_ == DragButton::None) return;

    const Vector2i pos{event.position()};
    const Vector2i delta = pos - lastMousePos_;
    lastMousePos_ = pos;

    const bool shiftHeld = bool(event.modifiers() & Modifier::Shift);
    const bool shouldPan = dragButton_ == DragButton::Right ||
                           dragButton_ == DragButton::Middle ||
                           (dragButton_ == DragButton::Left && shiftHeld);
    if (shouldPan) cubeScene_.arcBall().pan(delta);
    else cubeScene_.arcBall().rotate(delta);

    event.setAccepted();
}

void WebApp::scrollEvent(ScrollEvent& event) {
    const bool handledByImGui = imgui_.handleScrollEvent(event);
    const bool mouseOnUi = ImGui::GetIO().WantCaptureMouse &&
                           ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow);
    if (handledByImGui && mouseOnUi) {
        event.setAccepted();
        return;
    }

    cubeScene_.arcBall().zoom(event.offset().y());
    event.setAccepted();
}

void WebApp::textInputEvent(TextInputEvent& event) {
    imgui_.handleTextInputEvent(event);
}

void WebApp::tryEnqueueMove(Move move) {
    if (animManager_.isAnimating()) return;
    history_.push(move);
    animManager_.enqueueMove(move);
}

void WebApp::doUndo() {
    if (animManager_.isAnimating()) return;
    if (auto inv = history_.undo()) {
        animManager_.enqueueMove(*inv);
    }
}

void WebApp::doRedo() {
    if (animManager_.isAnimating()) return;
    if (auto m = history_.redo()) {
        animManager_.enqueueMove(*m);
    }
}

void WebApp::doAutoPlay() {
    if (animManager_.isAnimating()) return;
    if (pendingIndex_ >= static_cast<int>(pendingMoves_.size())) return;

    const int remaining = static_cast<int>(pendingMoves_.size()) - pendingIndex_;
    std::vector<Move> toPlay(pendingMoves_.begin() + pendingIndex_, pendingMoves_.end());
    for (auto& m : toPlay) history_.push(m);
    animManager_.enqueueSequence(toPlay);
    pendingIndex_ += remaining;
    LOG_INFO("Auto-Play: enqueueing {} moves", remaining);
}

void WebApp::doGoNext() {
    if (animManager_.isAnimating()) return;
    if (pendingIndex_ >= static_cast<int>(pendingMoves_.size())) return;

    Move m = pendingMoves_[pendingIndex_];
    history_.push(m);
    animManager_.enqueueMove(m);
    ++pendingIndex_;
    LOG_DEBUG("Go Next: {}", m.toChar());
}

void WebApp::doGoBack() {
    if (animManager_.isAnimating()) return;
    if (pendingIndex_ <= 0) return;

    --pendingIndex_;
    if (auto inv = history_.undo()) {
        animManager_.enqueueMove(*inv);
    }
    LOG_DEBUG("Go Back: pendingIndex={}", pendingIndex_);
}

} // namespace rubik
