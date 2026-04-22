#include "app/DesktopApp.h"
#include "solver/Solver.h"
#include "utils/FileIO.h"
#include "utils/Logger.h"

#include <algorithm>
#include <random>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <SDL.h>
#include <imgui.h>

namespace rubik {

using namespace Magnum;
using namespace Magnum::Math::Literals;

DesktopApp::DesktopApp(const Arguments& arguments)
    : Sdl2Application{arguments, Configuration{}
          .setTitle("Rubik's Cube Simulator")
          .setSize({1024, 768})
          .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    // On macOS, SDL may translate Ctrl+LeftClick to right click. We use
    // Shift+Left for pan; disable that emulation for consistent input.
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "0");

    Logger::init();
    LOG_INFO("Starting Rubik's Cube Simulator");

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::setClearColor(0xc8c8c8_rgbf);

    // Create ImGui context and load TTF font scaled for HiDPI (crisp text).
    // Default ProggyClean is a bitmap and scales poorly; use a vector font.
    ImGui::CreateContext();
    const Vector2 uiSize = Vector2{windowSize()}/dpiScaling();
    const float fontScale = Float(framebufferSize().x()) / std::max(Float(uiSize.x()), 1.0f);
    const float fontPixels = 16.0f * fontScale;

    std::string fontPath;
    if (char* basePath = SDL_GetBasePath()) {
        fontPath = std::string(basePath) + "fonts/DroidSans.ttf";
        SDL_free(basePath);
    }
    if (fontPath.empty()) fontPath = "fonts/DroidSans.ttf";

    ImFont* font = nullptr;
    if (!fontPath.empty()) {
        font = ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.c_str(), fontPixels);
    }
    if (!font) {
        ImFontConfig fontCfg;
        fontCfg.SizePixels = fontPixels;
        ImGui::GetIO().Fonts->AddFontDefault(&fontCfg);
    }
    imgui_ = ImGuiIntegration::Context(*ImGui::GetCurrentContext(),
        uiSize, windowSize(), framebufferSize());

    cubeScene_.setup(framebufferSize());
    cubeScene_.syncFromState(cubeState_);

    std::string audioPath = "audio.mp3";
    if (char* basePath = SDL_GetBasePath()) {
        audioPath = std::string(basePath) + "audio.mp3";
        SDL_free(basePath);
    }
    audioPlayer_.init(audioPath);

    animManager_.onMoveBegin = [this](Move m) {
        cubeScene_.beginFaceRotation(m.face, cubeState_);
        audioPlayer_.playMoveSound();
        LOG_DEBUG("Animation begin: {}", m.toChar());
    };

    animManager_.onMoveComplete = [this](Move m) {
        cubeState_.applyMove(m);
        cubeScene_.endFaceRotation(cubeState_);
        if (scrambleRemaining_ > 0) --scrambleRemaining_;
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
    ui_.onScramble = [this]() { doScramble(); };
    ui_.onSolve    = [this]() { doSolve(); };

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
    setSwapInterval(1);
}

void DesktopApp::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                                  GL::FramebufferClear::Depth);

    float dt = timeline_.previousFrameDuration();
    animManager_.moveDuration = (scrambleRemaining_ > 0) ? 0.05f : ui_.animationSpeed();
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

void DesktopApp::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    cubeScene_.reshape(event.framebufferSize());
    imgui_.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
                    event.windowSize(), event.framebufferSize());
}

void DesktopApp::keyPressEvent(KeyEvent& event) {
    if (imgui_.handleKeyPressEvent(event)) return;

    ctrlHeld_ = bool(event.modifiers() & Modifier::Ctrl);
    const bool commandHeld = bool(event.modifiers() & Modifier::Super);
    const bool shortcutHeld = ctrlHeld_ || commandHeld;
    bool shift = bool(event.modifiers() & Modifier::Shift);

    if (shortcutHeld) {
        if (event.key() == Key::Z) { doUndo(); event.setAccepted(); return; }
        if (event.key() == Key::Y) { doRedo(); event.setAccepted(); return; }
    }

    if (event.key() == Key::Equal) { cubeScene_.arcBall().zoom(1.0f); event.setAccepted(); return; }
    if (event.key() == Key::Minus) { cubeScene_.arcBall().zoom(-1.0f); event.setAccepted(); return; }
    if (!shortcutHeld && event.key() == Key::N) { doGoNext(); event.setAccepted(); return; }
    if (!shortcutHeld && event.key() == Key::P) { doGoBack(); event.setAccepted(); return; }

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

    if (char c = charForKey()) {
        if (auto m = Move::fromChar(c)) {
            tryEnqueueMove(*m);
            event.setAccepted();
        }
    }
}

void DesktopApp::keyReleaseEvent(KeyEvent& event) {
    if (imgui_.handleKeyReleaseEvent(event)) return;
    ctrlHeld_ = bool(event.modifiers() & Modifier::Ctrl);
}

void DesktopApp::pointerPressEvent(PointerEvent& event) {
    if (imgui_.handlePointerPressEvent(event) && ImGui::GetIO().WantCaptureMouse) return;

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

void DesktopApp::pointerReleaseEvent(PointerEvent& event) {
    if (imgui_.handlePointerReleaseEvent(event) && ImGui::GetIO().WantCaptureMouse) return;

    const bool releasingActiveButton =
        (event.pointer() == Pointer::MouseLeft  && dragButton_ == DragButton::Left) ||
        (event.pointer() == Pointer::MouseRight && dragButton_ == DragButton::Right) ||
        (event.pointer() == Pointer::MouseMiddle && dragButton_ == DragButton::Middle);
    if (!releasingActiveButton) return;

    dragButton_ = DragButton::None;
    event.setAccepted();
}

void DesktopApp::pointerMoveEvent(PointerMoveEvent& event) {
    if (imgui_.handlePointerMoveEvent(event) && ImGui::GetIO().WantCaptureMouse) return;

    if (dragButton_ == DragButton::None) return;

    Vector2i pos{event.position()};
    Vector2i delta = pos - lastMousePos_;
    lastMousePos_ = pos;

    const SDL_Keymod mods = SDL_GetModState();
    const bool shiftHeld = bool(event.modifiers() & Modifier::Shift) ||
                           (mods & KMOD_SHIFT);
    const bool shouldPan = dragButton_ == DragButton::Right ||
                           dragButton_ == DragButton::Middle ||
                           (dragButton_ == DragButton::Left && shiftHeld);
    if (shouldPan) cubeScene_.arcBall().pan(delta);
    else cubeScene_.arcBall().rotate(delta);

    event.setAccepted();
}

void DesktopApp::scrollEvent(ScrollEvent& event) {
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

void DesktopApp::textInputEvent(TextInputEvent& event) {
    imgui_.handleTextInputEvent(event);
}

void DesktopApp::tryEnqueueMove(Move move) {
    if (animManager_.isAnimating()) return;
    history_.push(move);
    animManager_.enqueueMove(move);
}

void DesktopApp::doUndo() {
    if (animManager_.isAnimating()) return;
    if (auto inv = history_.undo()) {
        animManager_.enqueueMove(*inv);
    }
}

void DesktopApp::doRedo() {
    if (animManager_.isAnimating()) return;
    if (auto m = history_.redo()) {
        animManager_.enqueueMove(*m);
    }
}

void DesktopApp::doAutoPlay() {
    if (animManager_.isAnimating()) return;
    if (pendingIndex_ >= static_cast<int>(pendingMoves_.size())) return;

    const int remaining = static_cast<int>(pendingMoves_.size()) - pendingIndex_;
    std::vector<Move> toPlay(pendingMoves_.begin() + pendingIndex_, pendingMoves_.end());
    for (auto& m : toPlay) history_.push(m);
    animManager_.enqueueSequence(toPlay);
    pendingIndex_ += remaining;
    LOG_INFO("Auto-Play: enqueueing {} moves", remaining);
}

void DesktopApp::doGoNext() {
    if (animManager_.isAnimating()) return;
    if (pendingIndex_ >= static_cast<int>(pendingMoves_.size())) return;

    Move m = pendingMoves_[pendingIndex_];
    history_.push(m);
    animManager_.enqueueMove(m);
    ++pendingIndex_;
    LOG_DEBUG("Go Next: {}", m.toChar());
}

void DesktopApp::doGoBack() {
    if (animManager_.isAnimating()) return;
    if (pendingIndex_ <= 0) return;

    --pendingIndex_;
    if (auto inv = history_.undo()) {
        animManager_.enqueueMove(*inv);
    }
    LOG_DEBUG("Go Back: pendingIndex={}", pendingIndex_);
}

void DesktopApp::doScramble() {
    if (animManager_.isAnimating()) return;

    pendingMoves_.clear();
    pendingIndex_ = 0;

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> faceDist(0, 5);
    std::uniform_int_distribution<int> dirDist(0, 1);
    static const Face faces[] = {Face::F, Face::B, Face::U, Face::D, Face::L, Face::R};

    constexpr int kScrambleLen = 30;
    std::vector<Move> moves;
    moves.reserve(kScrambleLen);
    Face lastFace = Face::F;
    bool hasLast = false;
    for (int i = 0; i < kScrambleLen; ++i) {
        Face f;
        do { f = faces[faceDist(rng)]; } while (hasLast && f == lastFace);
        Direction d = dirDist(rng) ? Direction::CW : Direction::CCW;
        moves.push_back(Move{f, d});
        lastFace = f;
        hasLast = true;
    }

    for (const auto& m : moves) history_.push(m);
    animManager_.enqueueSequence(moves);
    scrambleRemaining_ = static_cast<int>(moves.size());
    LOG_INFO("Scramble: enqueueing {} moves", moves.size());
}

void DesktopApp::doSolve() {
    if (animManager_.isAnimating()) return;

    if (cubeState_.isSolved()) {
        pendingMoves_.clear();
        pendingIndex_ = 0;
        LOG_INFO("Solve: cube already solved");
        return;
    }

    auto report = Solver::solveWithReport(cubeState_);
    if (!report.solved) {
        pendingMoves_.clear();
        pendingIndex_ = 0;
        LOG_WARN("Solver failed to reach a solved state; refusing to enqueue partial sequence");
        return;
    }

    pendingMoves_ = std::move(report.moves);
    pendingIndex_ = 0;
    LOG_INFO("Solver produced {} moves (use Auto-Play or Go Next to execute)",
             pendingMoves_.size());
}

} // namespace rubik
