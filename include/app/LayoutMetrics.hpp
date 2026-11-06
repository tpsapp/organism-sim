#pragma once

#include "sim/Common.hpp"

namespace app {

struct SidebarLayout {
    // Computes the maximum Y extent (in pixels) of the sidebar elements
    static constexpr int CalculateContentBottomY(bool has_notification, bool org_inspected) noexcept {
        int y = 20; // Top margin
        y += 28;    // Header title ("ORGANISM SIM") (24.0f)
        y += 20;    // Subtitle ("A-Life Multi-Tile Engine") (14.0f)
        if (has_notification) {
            y += 28; // Notification banner + spacing
        }
        y += 10; // Divider spacing

        // Status banner row (16.0f)
        y += 20;
        // Time Scale & Step counter row (14.0f)
        y += 20;
        y += 10; // Divider spacing

        // Grid occupancy (Label + counts on top row, bar below)
        y += 18;        // Label row (15.0f / 13.5f)
        y += 10 + 10;   // Bar (10px) + divider spacing (10px)

        // Population stats (2 columns: 4 text rows + graph)
        y += 20;        // Title ("POPULATION STATS") (15.0f)
        y += 18;        // Row 1: Total & Gestating (14.0f)
        y += 18;        // Row 2: Males & Females (14.0f)
        y += 18;        // Row 3: Food & Water (13.5f)
        y += 20;        // Row 4: Avg Age (13.5f)
        y += 42 + 8;    // Sparkline graph (42px) + spacing
        y += 10;        // Divider spacing

        // Manual Placement (2x2 grid: 2 button rows)
        y += 20; // Title ("MANUAL PLACEMENT") (15.0f)
        y += 36; // Row 1: Food & Water (btn_h = 30)
        y += 36; // Row 2: Male & Female (btn_h = 30)
        y += 10; // Divider spacing

        // Controls (2 columns: 4 text rows)
        y += 20; // Title ("CONTROLS") (15.0f)
        y += 17; // Row 1: Space & +/- (13.0f)
        y += 17; // Row 2: 1-4 & L-Click (13.0f)
        y += 17; // Row 3: F5/F9 & R-Drag (13.0f)
        y += 18; // Row 4: Wheel & C/R (13.0f)
        y += 10; // Divider spacing

        // Inspector Section
        y += 20; // Title ("ORGANISM INSPECTOR") (15.0f)
        if (org_inspected) {
            y += 8;  // Card padding
            y += 20; // ID / Gender (16.0f)
            y += 18; // Size / Damage (14.0f)
            y += 17; // Life force label (14.0f)
            y += 16; // Health bar (10px)
            y += 18; // Hunger & Thirst (14.0f)
            y += 18; // Calories & Age (14.0f)
            y += 18; // Scent (14.0f)
            y += 18; // Sight & Smell (13.0f)
            y += 18; // State (14.0f)
            y += 18; // Gestation (14.0f)
            y += 8;  // Bottom card margin
        } else {
            y += 50; // Empty placeholder prompt
        }

        return y;
    }
};

} // namespace app

