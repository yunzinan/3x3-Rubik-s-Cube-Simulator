#pragma once

#include "core/CubeState.h"
#include "core/History.h"

#include <Magnum/ImGuiIntegration/Context.h>

#include <array>
#include <functional>
#include <string>

namespace rubik {

// Dear ImGui overlay – control panel for the simulator.
class ImGuiUI {
public:
    // ── Callbacks (set by the application) ──────────────────────────────
    std::function<void()>              onReset;
    std::function<void()>              onResetView;
    std::function<void()>              onUndo;
    std::function<void()>              onRedo;
    std::function<void(const std::string&)> onLoadFile;
    std::function<void(const std::string&)> onSaveFile;
    std::function<void()> onClearSequence;
    std::function<void()> onAutoPlay;   // Play all remaining pending moves
    std::function<void()> onGoNext;     // Play one pending move
    std::function<void()> onGoBack;     // Undo the last executed pending move
    std::function<void()> onScramble;   // Generate a random scramble
    std::function<void()> onSolve;      // Compute a solution for the current state
    std::function<bool(const FaceletColor[6][9])> onApplyStateInput;

    // Draw all ImGui windows. Call between imgui.newFrame() and
    // imgui.drawFrame().
    // pendingCursor / pendingTotal describe the loaded-but-not-yet-played sequence.
    void draw(const History& history, bool isAnimating, float animSpeed,
              int pendingCursor, int pendingTotal);

    void setFilePathDefault(const std::string& path);

    // Returns the animation speed value set by the slider.
    float animationSpeed() const { return animSpeed_; }

    bool isInputPanelOpen() const { return showInputPanel_; }

private:
    float animSpeed_ = 0.35f;
    char filePathBuf_[256] = "sequence.json";

    // ── State input panel ───────────────────────────────────────────────
    bool showInputPanel_ = false;
    FaceletColor selectedColor_ = FaceletColor::White;
    FaceletColor inputFacelets_[6][9] = {}; // [face][sticker]
    bool inputInitialized_ = false;
    std::string inputError_;

    void initInputFacelets();
    void drawInputStatePanel(bool isAnimating);

    // ── Cross layout helpers ────────────────────────────────────────────
    // Face origins in a 11×12 sticker grid (row 0-10, col 0-11).
    //        col: 0  1  2  3  4  5  6  7  8  9 10 11
    // row 0-2:                         U  U  U
    // row 3:                     (gap)
    // row 4-6:    L  L  L        F  F  F  R  R  R  B  B  B
    // row 7:                     (gap)
    // row 8-10:                   D  D  D
    struct GridOrigin { int row, col; };
    static constexpr GridOrigin kFaceGridOrigin[6] = {
        {0, 3},  // U
        {8, 3},  // D
        {4, 3},  // F
        {4, 9},  // B
        {4, 0},  // L
        {4, 6},  // R
    };
};

} // namespace rubik
