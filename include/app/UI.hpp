#pragma once

#include "sim/Simulation.hpp"
#include "raylib.h"
#include <string>

namespace app {

class UI {
public:
    UI() = default;
    ~UI();

    void Init();
    void Cleanup();

    void DrawSidebar(const sim::Simulation& sim, sim::ActiveTool active_tool, float time_scale, bool paused, const sim::Organism* hovered_org = nullptr, const std::string& notification_msg = "");
    void DrawEndGameModal(const sim::Simulation& sim);

    void DrawCrispText(const char* text, float x, float y, float font_size, Color color) const;
    void DrawCrispText(const std::string& text, float x, float y, float font_size, Color color) const {
        DrawCrispText(text.c_str(), x, y, font_size, color);
    }

    [[nodiscard]] bool IsCustomFontLoaded() const noexcept { return custom_font_loaded_; }

private:
    Font font_{};
    bool custom_font_loaded_{false};
};

} // namespace app


