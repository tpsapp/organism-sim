#include "app/UI.hpp"
#include <string>
#include <sstream>
#include <iomanip>

namespace app {

UI::~UI() {
    Cleanup();
}

void UI::Init() {
    std::string app_dir = GetApplicationDirectory();
    std::string candidate_paths[] = {
        "assets/fonts/LiberationSans-Regular.ttf",
        "../assets/fonts/LiberationSans-Regular.ttf",
        app_dir + "assets/fonts/LiberationSans-Regular.ttf",
        app_dir + "../assets/fonts/LiberationSans-Regular.ttf",
        app_dir + "../share/organism_sim/fonts/LiberationSans-Regular.ttf",
        "/usr/local/share/organism_sim/fonts/LiberationSans-Regular.ttf",
        "/usr/share/organism_sim/fonts/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf"
    };

    for (const auto& path : candidate_paths) {
        if (FileExists(path.c_str())) {
            font_ = LoadFontEx(path.c_str(), 36, nullptr, 0);
            if (font_.texture.id > 0) {
                SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
                custom_font_loaded_ = true;
                break;
            }
        }
    }
}

void UI::Cleanup() {
    if (custom_font_loaded_) {
        UnloadFont(font_);
        custom_font_loaded_ = false;
    }
}

void UI::DrawCrispText(const char* text, float x, float y, float font_size, Color color) const {
    if (custom_font_loaded_) {
        DrawTextEx(font_, text, Vector2{x, y}, font_size, 1.0f, color);
    } else {
        DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(font_size), color);
    }
}

void UI::DrawSidebar(const sim::Simulation& sim, sim::ActiveTool active_tool, float time_scale, bool paused, const sim::Organism* hovered_org, const std::string& notification_msg) {
    int sx = sim::PLAYFIELD_WIDTH_PX;
    int sy = 0;
    int sw = sim::SIDEBAR_WIDTH_PX;
    int sh = sim::WINDOW_HEIGHT_PX;

    // Sidebar background
    DrawRectangle(sx, sy, sw, sh, Color{26, 28, 36, 255});

    int y = 20;
    auto draw_text_row = [&](const std::string& text, float font_size, Color col) {
        DrawCrispText(text, static_cast<float>(sx + 20), static_cast<float>(y), font_size, col);
        y += static_cast<int>(font_size + 6);
    };

    // Header
    DrawCrispText("ORGANISM SIM", static_cast<float>(sx + 20), static_cast<float>(y), 22.0f, Color{240, 240, 245, 255});
    y += 28;
    DrawCrispText("A-Life Multi-Tile Engine", static_cast<float>(sx + 20), static_cast<float>(y), 13.0f, Color{140, 145, 160, 255});
    y += 18;

    if (!notification_msg.empty()) {
        DrawRectangle(sx + 15, y, sw - 30, 24, Color{35, 75, 55, 255});
        DrawRectangleLines(sx + 15, y, sw - 30, 24, Color{46, 204, 113, 255});
        DrawCrispText(notification_msg, static_cast<float>(sx + 22), static_cast<float>(y + 5), 12.0f, Color{230, 255, 230, 255});
        y += 28;
    }

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 12;

    // Status Banner
    sim::GameStatus status = sim.GetGameStatus();
    if (status == sim::GameStatus::ONGOING) {
        if (paused) {
            draw_text_row("STATUS: PAUSED", 15, Color{241, 196, 15, 255});
        } else {
            draw_text_row("STATUS: RUNNING", 15, Color{46, 204, 113, 255});
        }
    } else if (status == sim::GameStatus::VICTORY) {
        draw_text_row("STATUS: VICTORY (75%+)", 16, Color{241, 196, 15, 255});
    } else {
        draw_text_row("STATUS: EXTINCTION", 16, Color{231, 76, 60, 255});
    }

    std::ostringstream ss_speed;
    ss_speed << "Time Scale: " << std::fixed << std::setprecision(0) << time_scale << "x";
    draw_text_row(ss_speed.str(), 14, Color{180, 185, 200, 255});

    // Step Counter Telemetry
    auto format_step_count = [](uint64_t count) -> std::string {
        std::string s = std::to_string(count);
        int insert_pos = static_cast<int>(s.length()) - 3;
        while (insert_pos > 0) {
            s.insert(insert_pos, ",");
            insert_pos -= 3;
        }
        return s;
    };
    draw_text_row("Steps: " + format_step_count(sim.GetStepCount()), 14, Color{180, 185, 200, 255});
    y += 6;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 12;

    // Grid Occupancy
    draw_text_row("GRID OCCUPANCY", 15, Color{220, 225, 240, 255});
    size_t occ_cells = sim.GetGrid().OccupiedCellCount();
    float occ_pct = sim.OccupancyPercentage();

    std::ostringstream ss_occ;
    ss_occ << occ_cells << " / 10,000 (" << std::fixed << std::setprecision(1) << occ_pct << "%)";
    draw_text_row(ss_occ.str(), 13, Color{160, 165, 180, 255});

    // Progress Bar (towards 75%)
    int bar_w = sw - 40;
    int bar_h = 12;
    DrawRectangle(sx + 20, y, bar_w, bar_h, Color{40, 42, 54, 255});
    float progress = std::clamp(occ_pct / 75.0f, 0.0f, 1.0f);
    DrawRectangle(sx + 20, y, static_cast<int>(bar_w * progress), bar_h, Color{52, 152, 219, 255});
    DrawRectangleLines(sx + 20, y, bar_w, bar_h, Color{80, 85, 105, 255});
    y += bar_h + 12;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 12;

    // Demographics
    draw_text_row("POPULATION STATS", 15, Color{220, 225, 240, 255});
    draw_text_row("Total Organisms: " + std::to_string(sim.GetEntities().ActiveOrganismCount()), 13, Color{200, 205, 220, 255});
    draw_text_row("Males: " + std::to_string(sim.MaleCount()), 13, Color{70, 150, 255, 255});
    draw_text_row("Females: " + std::to_string(sim.FemaleCount()), 13, Color{240, 220, 70, 255});
    draw_text_row("Gestating: " + std::to_string(sim.GestatingCount()), 13, Color{255, 120, 190, 255});

    std::ostringstream ss_age;
    ss_age << "Avg Age: " << std::fixed << std::setprecision(1) << sim.AverageAge() << " yrs";
    draw_text_row(ss_age.str(), 13, Color{180, 185, 200, 255});

    size_t food_cnt = 0;
    size_t water_cnt = 0;
    for (const auto& r : sim.GetEntities().GetResources()) {
        if (!r.alive) continue;
        if (r.type == sim::EntityType::FOOD) ++food_cnt;
        else ++water_cnt;
    }
    draw_text_row("Food: " + std::to_string(food_cnt) + " | Water: " + std::to_string(water_cnt), 13, Color{160, 165, 180, 255});
    y += 4;

    // Real-Time Demographic Dynamic Graph
    const auto& history = sim.GetPopulationHistory();
    if (history.size() >= 2) {
        int graph_x = sx + 20;
        int graph_y = y;
        int graph_w = sw - 40;
        int graph_h = 42;

        DrawRectangle(graph_x, graph_y, graph_w, graph_h, Color{20, 22, 28, 255});
        DrawRectangleLines(graph_x, graph_y, graph_w, graph_h, Color{45, 48, 62, 255});

        int max_val = 20;
        for (const auto& pt : history) {
            max_val = std::max(max_val, std::max(pt.males + pt.females, pt.food));
        }

        float dx = static_cast<float>(graph_w) / static_cast<float>(history.size() - 1);
        for (size_t i = 0; i < history.size() - 1; ++i) {
            float x1 = graph_x + static_cast<float>(i) * dx;
            float x2 = graph_x + static_cast<float>(i + 1) * dx;

            auto to_gy = [&](int val) -> float {
                float norm = std::clamp(static_cast<float>(val) / static_cast<float>(max_val), 0.0f, 1.0f);
                return static_cast<float>(graph_y + graph_h - 2) - norm * static_cast<float>(graph_h - 4);
            };

            // Food (Red line)
            DrawLineEx(Vector2{x1, to_gy(history[i].food)},
                       Vector2{x2, to_gy(history[i + 1].food)}, 1.0f, Color{180, 50, 50, 200});

            // Males (Blue line)
            DrawLineEx(Vector2{x1, to_gy(history[i].males)},
                       Vector2{x2, to_gy(history[i + 1].males)}, 1.5f, Color{70, 150, 255, 255});

            // Females (Yellow line)
            DrawLineEx(Vector2{x1, to_gy(history[i].females)},
                       Vector2{x2, to_gy(history[i + 1].females)}, 1.5f, Color{240, 220, 70, 255});
        }
        y += graph_h + 8;
    } else {
        y += 6;
    }

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 12;

    // Manual Placement Tools
    draw_text_row("MANUAL PLACEMENT", 15, Color{220, 225, 240, 255});

    auto draw_tool_item = [&](int key_num, const std::string& name, const std::string& limit_str, sim::ActiveTool tool, Color swatch_col, Color unselected_text_col) {
        bool selected = (active_tool == tool);

        Color box_bg = selected ? Color{42, 60, 92, 255} : Color{30, 32, 42, 255};
        Color border_col = selected ? Color{90, 160, 255, 255} : Color{48, 52, 68, 255};

        DrawRectangle(sx + 20, y, sw - 40, 32, box_bg);
        DrawRectangleLines(sx + 20, y, sw - 40, 32, border_col);
        if (selected) {
            DrawRectangleLines(sx + 21, y + 1, sw - 42, 30, Color{120, 185, 255, 180});
        }

        // Color Swatch Indicator (12x12 px square representing playfield entity color)
        int swatch_x = sx + 28;
        int swatch_y = y + 10;
        DrawRectangle(swatch_x, swatch_y, 12, 12, swatch_col);
        DrawRectangleLines(swatch_x, swatch_y, 12, 12, Color{220, 225, 240, 200});

        // Key Number & Name Label with High-Contrast Typography
        Color text_col = selected ? Color{255, 255, 255, 255} : unselected_text_col;
        std::string label = "[" + std::to_string(key_num) + "] " + name + " (" + limit_str + ")";
        DrawCrispText(label, static_cast<float>(sx + 48), static_cast<float>(y + 7), 13.0f, text_col);
        y += 36;
    };

    draw_tool_item(1, "Food", "Unlimited", sim::ActiveTool::FOOD, Color{118, 28, 28, 255}, Color{255, 130, 130, 255});
    draw_tool_item(2, "Water", "Unlimited", sim::ActiveTool::WATER, Color{125, 125, 128, 255}, Color{220, 235, 250, 255});
    draw_tool_item(3, "Male", std::to_string(sim.RemainingMalePlacements()) + "/10", sim::ActiveTool::MALE, Color{70, 150, 255, 255}, Color{110, 180, 255, 255});
    draw_tool_item(4, "Female", std::to_string(sim.RemainingFemalePlacements()) + "/10", sim::ActiveTool::FEMALE, Color{240, 220, 70, 255}, Color{255, 230, 100, 255});
    y += 6;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 12;

    // Controls Guide
    draw_text_row("CONTROLS", 15, Color{220, 225, 240, 255});
    draw_text_row("Space  : Pause / Resume", 12, Color{160, 165, 180, 255});
    draw_text_row("+ / -  : Adjust Speed (1-8x)", 12, Color{160, 165, 180, 255});
    draw_text_row("1 - 4  : Select Tool", 12, Color{160, 165, 180, 255});
    draw_text_row("L-Click: Place Entity", 12, Color{160, 165, 180, 255});
    draw_text_row("F5 / F9: Save / Load State", 12, Color{160, 165, 180, 255});
    draw_text_row("R-Drag : Pan Viewport", 12, Color{160, 165, 180, 255});
    draw_text_row("Wheel/C: Zoom / Reset Cam", 12, Color{160, 165, 180, 255});
    draw_text_row("R      : Reset Simulation", 12, Color{160, 165, 180, 255});
    y += 4;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 12;

    // Bottom Telemetry Inspector Card
    draw_text_row("ORGANISM INSPECTOR", 15, Color{220, 225, 240, 255});

    int card_y = y;
    int card_h = sh - card_y - 16;

    if (hovered_org != nullptr) {
        DrawRectangle(sx + 15, card_y, sw - 30, card_h, Color{32, 35, 46, 255});
        DrawRectangleLines(sx + 15, card_y, sw - 30, card_h, Color{65, 70, 90, 255});

        y += 8;
        Color gender_col = (hovered_org->gender == sim::Gender::MALE) ? Color{70, 150, 255, 255} : Color{240, 220, 70, 255};
        std::string gender_str = (hovered_org->gender == sim::Gender::MALE) ? "Male" : "Female";
        DrawCrispText("ID #" + std::to_string(hovered_org->id) + " (" + gender_str + ")", static_cast<float>(sx + 25), static_cast<float>(y), 15.0f, gender_col);
        y += 20;

        std::string size_str = "Size: " + std::to_string(hovered_org->w) + "x" + std::to_string(hovered_org->h) +
                               " | Dmg: " + std::to_string(hovered_org->damage);
        DrawCrispText(size_str, static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{180, 185, 200, 255});
        y += 18;

        // Health Bar
        std::ostringstream ss_hp;
        ss_hp << "Life Force: " << std::fixed << std::setprecision(1) << hovered_org->life_force << "/100";
        DrawCrispText(ss_hp.str(), static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{200, 205, 220, 255});
        y += 16;

        int hp_bar_w = sw - 50;
        DrawRectangle(sx + 25, y, hp_bar_w, 8, Color{45, 48, 60, 255});
        float hp_ratio = std::clamp(hovered_org->life_force / 100.0f, 0.0f, 1.0f);
        DrawRectangle(sx + 25, y, static_cast<int>(hp_bar_w * hp_ratio), 8, Color{46, 204, 113, 255});
        DrawRectangleLines(sx + 25, y, hp_bar_w, 8, Color{70, 75, 95, 255});
        y += 14;

        // Hunger and Thirst
        std::ostringstream ss_ht;
        ss_ht << "Hunger: " << std::fixed << std::setprecision(1) << hovered_org->hunger << "/10"
              << " | Thirst: " << std::fixed << std::setprecision(1) << hovered_org->thirst << "/10";
        DrawCrispText(ss_ht.str(), static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{180, 185, 200, 255});
        y += 18;

        // Calories & Age
        std::string cal_str = "Calories: " + std::to_string(hovered_org->calories) +
                              " (Tot: " + std::to_string(hovered_org->lifetime_calories) + ")" +
                              " | Age: " + std::to_string(hovered_org->age);
        DrawCrispText(cal_str, static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{180, 185, 200, 255});
        y += 18;

        // Scent & Sensory
        std::ostringstream ss_scent;
        ss_scent << "Scent: 0x" << std::hex << std::uppercase << hovered_org->scent << std::dec;
        DrawCrispText(ss_scent.str(), static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{160, 165, 180, 255});
        y += 18;

        std::string sensory_str = "Sight: " + std::to_string(hovered_org->sight_range) + " (Cyan)" +
                                  " | Smell: " + std::to_string(hovered_org->smell_range) + " (Violet)";
        DrawCrispText(sensory_str, static_cast<float>(sx + 25), static_cast<float>(y), 12.0f, Color{120, 210, 255, 255});
        y += 18;

        // State & Gestation
        std::string state_str = "State: ";
        switch (hovered_org->current_state) {
            case sim::FSMState::WANDER: state_str += "Wandering"; break;
            case sim::FSMState::CRITICAL_NEED: state_str += "Seeking Sustenance"; break;
            case sim::FSMState::SEEK_MATE: state_str += "Seeking Partner"; break;
        }
        DrawCrispText(state_str, static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{240, 200, 70, 255});
        y += 18;

        if (hovered_org->gestating) {
            std::ostringstream ss_gest;
            ss_gest << "Pregnant: " << std::fixed << std::setprecision(1) << hovered_org->gestation_timer << "s left";
            DrawCrispText(ss_gest.str(), static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{255, 120, 190, 255});
        } else {
            DrawCrispText("Gestation: None", static_cast<float>(sx + 25), static_cast<float>(y), 13.0f, Color{130, 135, 150, 255});
        }
    } else {
        DrawRectangle(sx + 15, card_y, sw - 30, card_h, Color{24, 26, 34, 255});
        DrawRectangleLines(sx + 15, card_y, sw - 30, card_h, Color{45, 48, 60, 255});
        DrawCrispText("Hover mouse over an", static_cast<float>(sx + 45), static_cast<float>(card_y + card_h / 2 - 14), 14.0f, Color{120, 125, 140, 255});
        DrawCrispText("organism to inspect stats", static_cast<float>(sx + 35), static_cast<float>(card_y + card_h / 2 + 6), 14.0f, Color{120, 125, 140, 255});
    }
}

} // namespace app
