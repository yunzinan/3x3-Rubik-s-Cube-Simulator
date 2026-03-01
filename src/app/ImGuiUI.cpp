#include "app/ImGuiUI.h"

#include <imgui.h>

namespace rubik {

void ImGuiUI::draw(const History& history, bool isAnimating, float /*animSpeed*/) {
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({320, 0}, ImGuiCond_FirstUseEver);

    ImGui::Begin("Rubik's Cube Controls");

    // --- Cube actions ---
    if (ImGui::Button("Reset Cube")) {
        if (onReset) onReset();
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

    // --- Status ---
    ImGui::Separator();
    ImGui::Text("Moves: %d / %d", history.cursor(),
                static_cast<int>(history.moves().size()));
    ImGui::Text("State: %s", isAnimating ? "Moving" : "Idle");

    ImGui::End();
}

} // namespace rubik
