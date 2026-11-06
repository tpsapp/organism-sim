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

    // Header
    DrawCrispText("ORGANISM SIM", static_cast<float>(sx + 20), static_cast<float>(y), 22.0f, Color{240, 240, 245, 255});
    y += 26;
    DrawCrispText("A-Life Multi-Tile Engine", static_cast<float>(sx + 20), static_cast<float>(y), 13.0f, Color{140, 145, 160, 255});
    y += 18;

    if (!notification_msg.empty()) {
        DrawRectangle(sx + 15, y, sw - 30, 24, Color{35, 75, 55, 255});
        DrawRectangleLines(sx + 15, y, sw - 30, 24, Color{46, 204, 113, 255});
        DrawCrispText(notification_msg, static_cast<float>(sx + 22), static_cast<float>(y + 5), 12.0f, Color{230, 255, 230, 255});
        y += 28;
    }

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 10;

    // Status Banner & Time Scale / Steps (Single compact row)
    sim::GameStatus status = sim.GetGameStatus();
    if (status == sim::GameStatus::ONGOING) {
        if (paused) {
            DrawCrispText("STATUS: PAUSED", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{241, 196, 15, 255});
        } else {
            DrawCrispText("STATUS: RUNNING", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{46, 204, 113, 255});
        }
    } else if (status == sim::GameStatus::VICTORY) {
        DrawCrispText("STATUS: VICTORY", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{241, 196, 15, 255});
    } else {
        DrawCrispText("STATUS: EXTINCTION", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{231, 76, 60, 255});
    }

    auto format_step_count = [](uint64_t count) -> std::string {
        std::string s = std::to_string(count);
        int insert_pos = static_cast<int>(s.length()) - 3;
        while (insert_pos > 0) {
            s.insert(insert_pos, ",");
            insert_pos -= 3;
        }
        return s;
    };

    std::ostringstream ss_meta;
    ss_meta << std::fixed << std::setprecision(0) << time_scale << "x | Step " << format_step_count(sim.GetStepCount());
    DrawCrispText(ss_meta.str(), static_cast<float>(sx + 145), static_cast<float>(y + 1), 12.0f, Color{180, 185, 200, 255});
    y += 18;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 10;

    // Grid Occupancy
    DrawCrispText("GRID OCCUPANCY", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{220, 225, 240, 255});
    size_t occ_cells = sim.GetGrid().OccupiedCellCount();
    float occ_pct = sim.OccupancyPercentage();

    std::ostringstream ss_occ;
    ss_occ << occ_cells << " (" << std::fixed << std::setprecision(1) << occ_pct << "%)";
    DrawCrispText(ss_occ.str(), static_cast<float>(sx + 155), static_cast<float>(y + 1), 12.0f, Color{160, 165, 180, 255});
    y += 16;

    // Progress Bar (towards 75%)
    int bar_w = sw - 40;
    int bar_h = 10;
    DrawRectangle(sx + 20, y, bar_w, bar_h, Color{40, 42, 54, 255});
    float progress = std::clamp(occ_pct / 75.0f, 0.0f, 1.0f);
    DrawRectangle(sx + 20, y, static_cast<int>(bar_w * progress), bar_h, Color{52, 152, 219, 255});
    DrawRectangleLines(sx + 20, y, bar_w, bar_h, Color{80, 85, 105, 255});
    y += bar_h + 10;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 10;

    // Population Stats (Reorganized into 2 Columns with Graph Below)
    DrawCrispText("POPULATION STATS", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{220, 225, 240, 255});
    y += 18;

    // Row 1: Total & Gestating
    DrawCrispText("Total: " + std::to_string(sim.GetEntities().ActiveOrganismCount()), static_cast<float>(sx + 20), static_cast<float>(y), 12.5f, Color{200, 205, 220, 255});
    DrawCrispText("Gestating: " + std::to_string(sim.GestatingCount()), static_cast<float>(sx + 145), static_cast<float>(y), 12.5f, Color{255, 120, 190, 255});
    y += 16;

    // Row 2: Males & Females
    DrawCrispText("Males: " + std::to_string(sim.MaleCount()), static_cast<float>(sx + 20), static_cast<float>(y), 12.5f, Color{70, 150, 255, 255});
    DrawCrispText("Females: " + std::to_string(sim.FemaleCount()), static_cast<float>(sx + 145), static_cast<float>(y), 12.5f, Color{240, 220, 70, 255});
    y += 16;

    // Row 3: Food / Water & Avg Age
    size_t food_cnt = 0;
    size_t water_cnt = 0;
    for (const auto& r : sim.GetEntities().GetResources()) {
        if (!r.alive) continue;
        if (r.type == sim::EntityType::FOOD) ++food_cnt;
        else ++water_cnt;
    }
    DrawCrispText("Food: " + std::to_string(food_cnt) + " | Wtr: " + std::to_string(water_cnt), static_cast<float>(sx + 20), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    std::ostringstream ss_age;
    ss_age << "Avg Age: " << std::fixed << std::setprecision(1) << sim.AverageAge() << " yrs";
    DrawCrispText(ss_age.str(), static_cast<float>(sx + 145), static_cast<float>(y), 11.5f, Color{180, 185, 200, 255});
    y += 18;

    // Real-Time Demographic Dynamic Graph (Below the Two Columns)
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
    y += 10;

    // Manual Placement Tools (2x2 Grid)
    DrawCrispText("MANUAL PLACEMENT", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{220, 225, 240, 255});
    y += 18;

    auto draw_compact_tool_button = [&](int bx, int by, int bw, int bh, int key_num, const std::string& name, const std::string& limit_str, sim::ActiveTool tool, Color swatch_col, Color unselected_text_col) {
        bool selected = (active_tool == tool);

        Color box_bg = selected ? Color{42, 60, 92, 255} : Color{30, 32, 42, 255};
        Color border_col = selected ? Color{90, 160, 255, 255} : Color{48, 52, 68, 255};

        DrawRectangle(bx, by, bw, bh, box_bg);
        DrawRectangleLines(bx, by, bw, bh, border_col);
        if (selected) {
            DrawRectangleLines(bx + 1, by + 1, bw - 2, bh - 2, Color{120, 185, 255, 180});
        }

        // Swatch
        DrawRectangle(bx + 7, by + 8, 11, 11, swatch_col);
        DrawRectangleLines(bx + 7, by + 8, 11, 11, Color{220, 225, 240, 200});

        // Label
        Color text_col = selected ? Color{255, 255, 255, 255} : unselected_text_col;
        std::string label = "[" + std::to_string(key_num) + "] " + name + " (" + limit_str + ")";
        DrawCrispText(label, static_cast<float>(bx + 23), static_cast<float>(by + 6), 11.5f, text_col);
    };

    int btn_w = (sw - 40 - 8) / 2; // 116px
    int btn_h = 28;

    // Row 1: Food & Water
    draw_compact_tool_button(sx + 20, y, btn_w, btn_h, 1, "Food", "Inf", sim::ActiveTool::FOOD, Color{118, 28, 28, 255}, Color{255, 130, 130, 255});
    draw_compact_tool_button(sx + 20 + btn_w + 8, y, btn_w, btn_h, 2, "Water", "Inf", sim::ActiveTool::WATER, Color{125, 125, 128, 255}, Color{220, 235, 250, 255});
    y += btn_h + 6;

    // Row 2: Male & Female
    draw_compact_tool_button(sx + 20, y, btn_w, btn_h, 3, "Male", std::to_string(sim.RemainingMalePlacements()), sim::ActiveTool::MALE, Color{70, 150, 255, 255}, Color{110, 180, 255, 255});
    draw_compact_tool_button(sx + 20 + btn_w + 8, y, btn_w, btn_h, 4, "Female", std::to_string(sim.RemainingFemalePlacements()), sim::ActiveTool::FEMALE, Color{240, 220, 70, 255}, Color{255, 230, 100, 255});
    y += btn_h + 6;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 10;

    // Controls Guide (Two Columns)
    DrawCrispText("CONTROLS", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{220, 225, 240, 255});
    y += 18;

    // Row 1
    DrawCrispText("Space : Pause/Play", static_cast<float>(sx + 20), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    DrawCrispText("+ / - : Speed 1-8x", static_cast<float>(sx + 145), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    y += 15;

    // Row 2
    DrawCrispText("1 - 4 : Select Tool", static_cast<float>(sx + 20), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    DrawCrispText("L-Click: Place", static_cast<float>(sx + 145), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    y += 15;

    // Row 3
    DrawCrispText("F5/F9 : Save/Load", static_cast<float>(sx + 20), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    DrawCrispText("R-Drag : Pan View", static_cast<float>(sx + 145), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    y += 15;

    // Row 4
    DrawCrispText("Wheel : Zoom View", static_cast<float>(sx + 20), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    DrawCrispText("C / R : Cam/Reset", static_cast<float>(sx + 145), static_cast<float>(y), 11.5f, Color{160, 165, 180, 255});
    y += 16;

    DrawLine(sx + 20, y, sx + sw - 20, y, Color{50, 52, 65, 255});
    y += 10;

    // Bottom Telemetry Inspector Card
    DrawCrispText("ORGANISM INSPECTOR", static_cast<float>(sx + 20), static_cast<float>(y), 14.0f, Color{220, 225, 240, 255});
    y += 18;

    int card_y = y;
    int card_h = sh - card_y - 12;

    if (hovered_org != nullptr) {
        DrawRectangle(sx + 15, card_y, sw - 30, card_h, Color{32, 35, 46, 255});
        DrawRectangleLines(sx + 15, card_y, sw - 30, card_h, Color{65, 70, 90, 255});

        y += 8;
        Color gender_col = (hovered_org->gender == sim::Gender::MALE) ? Color{70, 150, 255, 255} : Color{240, 220, 70, 255};
        std::string gender_str = (hovered_org->gender == sim::Gender::MALE) ? "Male" : "Female";
        DrawCrispText("ID #" + std::to_string(hovered_org->id) + " (" + gender_str + ")", static_cast<float>(sx + 25), static_cast<float>(y), 15.0f, gender_col);
        y += 18;

        std::string size_str = "Size: " + std::to_string(hovered_org->w) + "x" + std::to_string(hovered_org->h) +
                               " | Dmg: " + std::to_string(hovered_org->damage);
        DrawCrispText(size_str, static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{180, 185, 200, 255});
        y += 16;

        // Health Bar
        std::ostringstream ss_hp;
        ss_hp << "Life Force: " << std::fixed << std::setprecision(1) << hovered_org->life_force << "/100";
        DrawCrispText(ss_hp.str(), static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{200, 205, 220, 255});
        y += 15;

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
        DrawCrispText(ss_ht.str(), static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{180, 185, 200, 255});
        y += 16;

        // Calories & Age
        std::string cal_str = "Calories: " + std::to_string(hovered_org->calories) +
                              " (Tot: " + std::to_string(hovered_org->lifetime_calories) + ")" +
                              " | Age: " + std::to_string(hovered_org->age);
        DrawCrispText(cal_str, static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{180, 185, 200, 255});
        y += 16;

        // Scent & Sensory
        std::ostringstream ss_scent;
        ss_scent << "Scent: 0x" << std::hex << std::uppercase << hovered_org->scent << std::dec;
        DrawCrispText(ss_scent.str(), static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{160, 165, 180, 255});
        y += 16;

        std::string sensory_str = "Sight: " + std::to_string(hovered_org->sight_range) + " (Cyan)" +
                                  " | Smell: " + std::to_string(hovered_org->smell_range) + " (Violet)";
        DrawCrispText(sensory_str, static_cast<float>(sx + 25), static_cast<float>(y), 11.5f, Color{120, 210, 255, 255});
        y += 16;

        // State & Gestation
        std::string state_str = "State: ";
        switch (hovered_org->current_state) {
            case sim::FSMState::WANDER: state_str += "Wandering"; break;
            case sim::FSMState::CRITICAL_NEED: state_str += "Seeking Sustenance"; break;
            case sim::FSMState::SEEK_MATE: state_str += "Seeking Partner"; break;
        }
        DrawCrispText(state_str, static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{240, 200, 70, 255});
        y += 16;

        if (hovered_org->gestating) {
            std::ostringstream ss_gest;
            ss_gest << "Pregnant: " << std::fixed << std::setprecision(1) << hovered_org->gestation_timer << "s left";
            DrawCrispText(ss_gest.str(), static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{255, 120, 190, 255});
        } else {
            DrawCrispText("Gestation: None", static_cast<float>(sx + 25), static_cast<float>(y), 12.5f, Color{130, 135, 150, 255});
        }
    } else {
        DrawRectangle(sx + 15, card_y, sw - 30, card_h, Color{24, 26, 34, 255});
        DrawRectangleLines(sx + 15, card_y, sw - 30, card_h, Color{45, 48, 60, 255});
        DrawCrispText("Hover mouse over an", static_cast<float>(sx + 45), static_cast<float>(card_y + card_h / 2 - 14), 14.0f, Color{120, 125, 140, 255});
        DrawCrispText("organism to inspect stats", static_cast<float>(sx + 35), static_cast<float>(card_y + card_h / 2 + 6), 14.0f, Color{120, 125, 140, 255});
    }
}

} // namespace app
