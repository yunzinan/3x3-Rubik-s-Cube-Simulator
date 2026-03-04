#include "app/ImGuiUI.h"

#include <cstring>
#include <imgui.h>

namespace rubik {

void ImGuiUI::setFilePathDefault(const std::string& path) {
    const std::size_t maxLen = sizeof(filePathBuf_) - 1;
    std::strncpy(filePathBuf_, path.c_str(), maxLen);
    filePathBuf_[maxLen] = '\0';
}

void ImGuiUI::draw(const History& history, bool isAnimating, float /*animSpeed*/) {
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

    // --- Status ---
    ImGui::Separator();
    ImGui::Text("Moves: %d / %d", history.cursor(),
                static_cast<int>(history.moves().size()));
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
    ImGui::Spacing();
    ImGui::TextWrapped("View (camera):");
    ImGui::BulletText("Left drag  -- orbit / rotate view");
    ImGui::BulletText("Shift+Left or Right drag  -- pan view");
    ImGui::BulletText("Scroll wheel  -- zoom in / out");
    ImGui::BulletText("Reset View button  -- restore default camera");
    ImGui::End();
}

} // namespace rubik
