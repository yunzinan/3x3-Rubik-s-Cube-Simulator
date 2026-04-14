#pragma once

#include "core/History.h"

#include <Magnum/ImGuiIntegration/Context.h>

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

    // Draw all ImGui windows. Call between imgui.newFrame() and
    // imgui.drawFrame().
    // pendingCursor / pendingTotal describe the loaded-but-not-yet-played sequence.
    void draw(const History& history, bool isAnimating, float animSpeed,
              int pendingCursor, int pendingTotal);

    void setFilePathDefault(const std::string& path);

    // Returns the animation speed value set by the slider.
    float animationSpeed() const { return animSpeed_; }

private:
    float animSpeed_ = 0.35f;
    char filePathBuf_[256] = "sequence.json";
};

} // namespace rubik
