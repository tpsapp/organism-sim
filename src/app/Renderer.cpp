#include "app/Renderer.hpp"
#include <algorithm>

namespace app {

Color Renderer::GetOrganismColor(sim::Gender gender, float life_force) {
    float health_ratio = std::clamp(life_force / 100.0f, 0.0f, 1.0f);

    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;

    if (gender == sim::Gender::MALE) {
        // Full health: Vibrant Blue (50, 130, 240) -> Dying: Dark Gray/Charcoal (30, 35, 50)
        r = static_cast<unsigned char>(30 + health_ratio * (50 - 30));
        g = static_cast<unsigned char>(35 + health_ratio * (130 - 35));
        b = static_cast<unsigned char>(50 + health_ratio * (240 - 50));
    } else {
        // Full health: Vibrant Yellow (240, 210, 40) -> Dying: Dark Gray/Olive (50, 45, 30)
        r = static_cast<unsigned char>(50 + health_ratio * (240 - 50));
        g = static_cast<unsigned char>(45 + health_ratio * (210 - 45));
        b = static_cast<unsigned char>(30 + health_ratio * (40 - 30));
    }

    return Color{r, g, b, 255};
}

void Renderer::DrawGridLines() {
    // Draw subtle grid lines
    Color grid_col = Color{32, 32, 38, 255};
    for (int x = 0; x <= sim::GRID_WIDTH; ++x) {
        DrawLine(x * sim::CELL_SIZE_PX, 0, x * sim::CELL_SIZE_PX, sim::PLAYFIELD_HEIGHT_PX, grid_col);
    }
    for (int y = 0; y <= sim::GRID_HEIGHT; ++y) {
        DrawLine(0, y * sim::CELL_SIZE_PX, sim::PLAYFIELD_WIDTH_PX, y * sim::CELL_SIZE_PX, grid_col);
    }
}

void Renderer::DrawResources(const sim::Simulation& sim) {
    for (const auto& res : sim.GetEntities().GetResources()) {
        if (!res.alive) continue;
        int px = res.pos.x * sim::CELL_SIZE_PX;
        int py = res.pos.y * sim::CELL_SIZE_PX;

        if (res.type == sim::EntityType::FOOD) {
            // Food: 50% Darkened Red (118, 28, 28) for high contrast against brighter organisms
            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{118, 28, 28, 255});
        } else if (res.type == sim::EntityType::WATER) {
            // Water: 50% Darkened White/Gray (125, 125, 128) for high contrast against brighter organisms
            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{125, 125, 128, 255});
        }
    }
}

void Renderer::DrawOrganisms(const sim::Simulation& sim, const sim::Organism* hovered_org) {
    for (const auto& org : sim.GetEntities().GetOrganisms()) {
        if (!org.alive) continue;
        int px = org.pos.x * sim::CELL_SIZE_PX;
        int py = org.pos.y * sim::CELL_SIZE_PX;
        int pw = org.w * sim::CELL_SIZE_PX;
        int ph = org.h * sim::CELL_SIZE_PX;

        Color body_color = GetOrganismColor(org.gender, org.life_force);
        DrawRectangle(px + 1, py + 1, pw - 2, ph - 2, body_color);

        // Bounding outline for visual clarity
        bool is_hovered = (hovered_org != nullptr && hovered_org->id == org.id);
        if (is_hovered) {
            DrawRectangleLines(px, py, pw, ph, Color{255, 255, 255, 255});
            DrawRectangleLines(px - 1, py - 1, pw + 2, ph + 2, Color{241, 196, 15, 255});
        } else {
            DrawRectangleLines(px, py, pw, ph, Color{15, 15, 20, 220});
        }

        // If gestating, draw pink dot badge in center
        if (org.gestating) {
            int cx = px + pw / 2;
            int cy = py + ph / 2;
            DrawCircle(cx, cy, 2.5f, Color{255, 105, 180, 255});
        }
    }
}

void Renderer::DrawPlayfield(const sim::Simulation& sim, const sim::Organism* hovered_org) {
    // Background
    DrawRectangle(0, 0, sim::PLAYFIELD_WIDTH_PX, sim::PLAYFIELD_HEIGHT_PX, Color{22, 22, 26, 255});

    DrawGridLines();
    DrawResources(sim);
    DrawOrganisms(sim, hovered_org);

    // Border separating playfield and HUD sidebar
    DrawLine(sim::PLAYFIELD_WIDTH_PX, 0, sim::PLAYFIELD_WIDTH_PX, sim::WINDOW_HEIGHT_PX, Color{60, 60, 75, 255});
}

} // namespace app

