#include "app/ImGuiUI.h"

#include <cstring>
#include <imgui.h>

namespace rubik {

ImU32 faceletColorToImU32(FaceletColor c) {
    switch (c) {
        case FaceletColor::White:  return IM_COL32(255, 255, 255, 255);
        case FaceletColor::Yellow: return IM_COL32(255, 237, 0,   255);
        case FaceletColor::Green:  return IM_COL32(0,   158, 96,  255);
        case FaceletColor::Blue:   return IM_COL32(0,   81,  186, 255);
        case FaceletColor::Red:    return IM_COL32(196, 30,  58,  255);
        case FaceletColor::Orange: return IM_COL32(255, 69,  0,   255);
    }
    return IM_COL32(0, 0, 0, 255);
}

static ImU32 faceletHoverColor(FaceletColor c) {
    switch (c) {
        case FaceletColor::White:  return IM_COL32(230, 230, 230, 255);
        case FaceletColor::Yellow: return IM_COL32(220, 205, 0,   255);
        case FaceletColor::Green:  return IM_COL32(0,   130, 75,  255);
        case FaceletColor::Blue:   return IM_COL32(0,   65,  160, 255);
        case FaceletColor::Red:    return IM_COL32(170, 20,  45,  255);
        case FaceletColor::Orange: return IM_COL32(220, 55,  0,   255);
    }
    return IM_COL32(0, 0, 0, 255);
}

static ImU32 faceletActiveColor(FaceletColor c) {
    switch (c) {
        case FaceletColor::White:  return IM_COL32(200, 200, 200, 255);
        case FaceletColor::Yellow: return IM_COL32(190, 175, 0,   255);
        case FaceletColor::Green:  return IM_COL32(0,   110, 60,  255);
        case FaceletColor::Blue:   return IM_COL32(0,   50,  140, 255);
        case FaceletColor::Red:    return IM_COL32(145, 15,  35,  255);
        case FaceletColor::Orange: return IM_COL32(195, 45,  0,   255);
    }
    return IM_COL32(0, 0, 0, 255);
}

void ImGuiUI::setFilePathDefault(const std::string& path) {
    const std::size_t maxLen = sizeof(filePathBuf_) - 1;
    std::strncpy(filePathBuf_, path.c_str(), maxLen);
    filePathBuf_[maxLen] = '\0';
}

void ImGuiUI::initInputFacelets() {
    // Initialize to solved state: each face is uniform color.
    for (int f = 0; f < 6; ++f) {
        FaceletColor c;
        switch (f) {
            case kFaceletU: c = FaceletColor::White;  break;
            case kFaceletD: c = FaceletColor::Yellow; break;
            case kFaceletF: c = FaceletColor::Green;  break;
            case kFaceletB: c = FaceletColor::Blue;   break;
            case kFaceletL: c = FaceletColor::Orange;  break;
            case kFaceletR: c = FaceletColor::Red;     break;
            default: c = FaceletColor::White; break;
        }
        for (int s = 0; s < 9; ++s)
            inputFacelets_[f][s] = c;
    }
    inputError_.clear();
    inputInitialized_ = true;
}

void ImGuiUI::drawInputStatePanel(bool isAnimating) {
    if (!showInputPanel_) return;

    ImGui::SetNextWindowSize({460, 580}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Input Cube State", &showInputPanel_);

    ImGui::TextWrapped("Select a color, then click stickers to paint them.");
    ImGui::TextWrapped("Use the standard orientation: White=U, Yellow=D, Green=F, Blue=B, Red=R, Orange=L.");
    ImGui::Spacing();

    // ── Color palette ──
    ImGui::Text("Color:");
    ImGui::SameLine();
    constexpr FaceletColor palette[] = {
        FaceletColor::White, FaceletColor::Yellow, FaceletColor::Green,
        FaceletColor::Blue,  FaceletColor::Red,    FaceletColor::Orange
    };
    constexpr const char* colorNames[] = {"W", "Y", "G", "B", "R", "O"};
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ImGui::SameLine();
        bool selected = (selectedColor_ == palette[i]);
        ImU32 col = selected ? faceletActiveColor(palette[i]) : faceletColorToImU32(palette[i]);
        ImGui::PushStyleColor(ImGuiCol_Button,        col);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  faceletHoverColor(palette[i]));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   faceletActiveColor(palette[i]));
        if (selected) {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 255));
        }
        ImGui::PushID(i);
        if (ImGui::Button(colorNames[i], {30, 24}))
            selectedColor_ = palette[i];
        ImGui::PopID();
        if (selected) {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(); // border
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::Spacing();

    // ── Cross layout ──
    constexpr float stickerSize = 28.0f;
    constexpr float gap = 2.0f;
    ImVec2 base = ImGui::GetCursorScreenPos();

    auto stickerScreenPos = [&](int faceIdx, int sticker) -> ImVec2 {
        int row = sticker / 3, col = sticker % 3;
        int gRow = kFaceGridOrigin[faceIdx].row + row;
        int gCol = kFaceGridOrigin[faceIdx].col + col;
        return {base.x + gCol * (stickerSize + gap),
                base.y + gRow * (stickerSize + gap)};
    };

    // Draw all 54 stickers.
    for (int f = 0; f < 6; ++f) {
        for (int s = 0; s < 9; ++s) {
            ImVec2 pos = stickerScreenPos(f, s);
            ImGui::SetCursorScreenPos(pos);
            ImGui::PushStyleColor(ImGuiCol_Button,         faceletColorToImU32(inputFacelets_[f][s]));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  faceletHoverColor(inputFacelets_[f][s]));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,   faceletActiveColor(inputFacelets_[f][s]));
            ImGui::PushID(f * 100 + s);
            char label[8];
            std::snprintf(label, sizeof(label), "##%d%d", f, s);
            if (ImGui::Button(label, {stickerSize, stickerSize}))
                inputFacelets_[f][s] = selectedColor_;
            ImGui::PopID();
            ImGui::PopStyleColor(3);
        }
    }

    // Advance cursor past the cross layout (11 rows).
    ImGui::SetCursorScreenPos({base.x, base.y + 11 * (stickerSize + gap) + 4});
    ImGui::Separator();

    // ── Buttons ──
    if (ImGui::Button("Reset to Solved")) {
        initInputFacelets();
    }
    ImGui::SameLine();

    ImGui::BeginDisabled(isAnimating);
    if (ImGui::Button("Apply")) {
        // Validate.
        int counts[6] = {};
        for (int f = 0; f < 6; ++f)
            for (int s = 0; s < 9; ++s)
                ++counts[int(inputFacelets_[f][s])];
        bool valid = true;
        for (int i = 0; i < 6; ++i) {
            if (counts[i] != 9) {
                inputError_ = "Each color must appear exactly 9 times. Invalid counts.";
                valid = false;
                break;
            }
        }
        if (valid) {
            if (onApplyStateInput && onApplyStateInput(inputFacelets_)) {
                inputError_.clear();
                showInputPanel_ = false;
            } else {
                inputError_ = "Invalid cube state: impossible cubie color combination.";
            }
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        showInputPanel_ = false;
    }

    // Error message.
    if (!inputError_.empty()) {
        ImGui::Spacing();
        ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.0f}, "%s", inputError_.c_str());
    }

    ImGui::End();
}

void ImGuiUI::draw(const History& history, bool isAnimating, float /*animSpeed*/,
                   int pendingCursor, int pendingTotal) {
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({320, 0}, ImGuiCond_FirstUseEver);

    ImGui::Begin("Rubik's Cube Controls");

    // --- Cube actions ---
    if (ImGui::Button("Reset Cube")) {
        if (onReset) onReset();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        if (onResetView) onResetView();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(isAnimating || !history.canUndo());
    if (ImGui::Button("Undo (Ctrl+Z)")) {
        if (onUndo) onUndo();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(isAnimating || !history.canRedo());
    if (ImGui::Button("Redo (Ctrl+Y)")) {
        if (onRedo) onRedo();
    }
    ImGui::EndDisabled();

    // --- Animation speed ---
    ImGui::SliderFloat("Move Duration (s)", &animSpeed_, 0.05f, 1.5f, "%.2f");

    ImGui::Separator();

    // --- Scramble, Solve & Input State ---
    ImGui::BeginDisabled(isAnimating);
    if (ImGui::Button("Scramble")) {
        if (onScramble) onScramble();
    }
    ImGui::SameLine();
    if (ImGui::Button("Solve!")) {
        if (onSolve) onSolve();
    }
    ImGui::SameLine();
    if (ImGui::Button("Input State")) {
        if (!inputInitialized_) initInputFacelets();
        showInputPanel_ = true;
        inputError_.clear();
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    // --- File I/O ---
    ImGui::InputText("File", filePathBuf_, sizeof(filePathBuf_));

    ImGui::BeginDisabled(isAnimating);
    if (ImGui::Button("Load Sequence")) {
        if (onLoadFile) onLoadFile(filePathBuf_);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Save History")) {
        if (onSaveFile) onSaveFile(filePathBuf_);
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(isAnimating);
    if (ImGui::Button("Clear Sequence")) {
        if (onClearSequence) onClearSequence();
    }
    ImGui::EndDisabled();

    // --- Playback controls ---
    ImGui::Separator();

    const bool hasPending = pendingCursor < pendingTotal;
    const bool hasExecuted = pendingCursor > 0;

    ImGui::BeginDisabled(isAnimating || !hasPending);
    if (ImGui::Button("Auto-Play")) {
        if (onAutoPlay) onAutoPlay();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(isAnimating || !hasPending);
    if (ImGui::Button("Go Next (N)")) {
        if (onGoNext) onGoNext();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(isAnimating || !hasExecuted);
    if (ImGui::Button("Go Back (P)")) {
        if (onGoBack) onGoBack();
    }
    ImGui::EndDisabled();

    // --- Status ---
    ImGui::Separator();
    ImGui::Text("Executed: %d / %d", history.cursor(),
                static_cast<int>(history.moves().size()));
    ImGui::Text("Pending:  %d / %d", pendingCursor, pendingTotal);
    ImGui::Text("State: %s", isAnimating ? "Moving" : "Idle");

    ImGui::End();

    // --- Guide (top-right) ---
    const float guideWidth = 280.0f;
    const float pad = 10.0f;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(displaySize.x - guideWidth - pad, pad),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(guideWidth, 0), ImGuiCond_FirstUseEver); // 0 = auto height

    ImGui::Begin("Guide", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::TextWrapped("Keyboard (cube moves):");
    ImGui::BulletText("f / b / u / d / l / r  -- rotate face clockwise");
    ImGui::BulletText("Shift + key  -- rotate face counterclockwise");
    ImGui::BulletText("Ctrl+Z  -- Undo");
    ImGui::BulletText("Ctrl+Y  -- Redo");
    ImGui::BulletText("N  -- Go Next");
    ImGui::BulletText("P  -- Go Back");
    ImGui::Spacing();
    ImGui::TextWrapped("View (camera):");
    ImGui::BulletText("Left drag  -- orbit / rotate view");
    ImGui::BulletText("Shift+Left or Right drag  -- pan view");
    ImGui::BulletText("Scroll wheel  -- zoom in / out");
    ImGui::BulletText("Reset View button  -- restore default camera");
    ImGui::End();

    // --- Input State panel ---
    drawInputStatePanel(isAnimating);
}

} // namespace rubik
