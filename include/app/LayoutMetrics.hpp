#pragma once

#include "sim/Common.hpp"

namespace app {

struct SidebarLayout {
    // Computes the maximum Y extent (in pixels) of the sidebar elements
    static constexpr int CalculateContentBottomY(bool has_notification, bool org_inspected) noexcept {
        int y = 20; // Top margin
        y += 26;    // Header title ("ORGANISM SIM")
        y += 18;    // Subtitle ("A-Life Multi-Tile Engine")
        if (has_notification) {
            y += 28; // Notification banner + spacing
        }
        y += 10; // Divider spacing

        // Status row (Status, Speed, Steps on compact row)
        y += 20;
        y += 10; // Divider spacing

        // Grid occupancy (Label + counts on top row, bar below)
        y += 18;        // Label row
        y += 10 + 10;   // Bar (10px) + divider spacing (10px)

        // Population stats (2 columns: 3 text rows + graph)
        y += 20;        // Title ("POPULATION STATS")
        y += 18;        // Row 1: Total & Gestating
        y += 18;        // Row 2: Males & Females
        y += 20;        // Row 3: Food/Water & Avg Age
        y += 42 + 8;    // Sparkline graph (42px) + spacing
        y += 10;        // Divider spacing

        // Manual Placement (2x2 grid: 2 button rows)
        y += 20; // Title ("MANUAL PLACEMENT")
        y += 32; // Row 1: Food & Water
        y += 32; // Row 2: Male & Female
        y += 10; // Divider spacing

        // Controls (2 columns: 4 text rows)
        y += 18; // Title ("CONTROLS")
        y += 16; // Row 1: Space & +/-
        y += 16; // Row 2: 1-4 & L-Click
        y += 16; // Row 3: F5/F9 & R-Drag
        y += 16; // Row 4: Wheel & C/R
        y += 10; // Divider spacing

        // Inspector Section
        y += 20; // Title ("ORGANISM INSPECTOR")
        if (org_inspected) {
            y += 8;  // Card padding
            y += 20; // ID / Gender
            y += 18; // Size / Damage
            y += 16; // Life force label
            y += 14; // Health bar
            y += 18; // Hunger & Thirst
            y += 18; // Calories & Age
            y += 18; // Scent
            y += 18; // Sight & Smell
            y += 18; // State
            y += 18; // Gestation
            y += 8;  // Bottom card margin
        } else {
            y += 50; // Empty placeholder prompt
        }

        return y;
    }
};

} // namespace app
