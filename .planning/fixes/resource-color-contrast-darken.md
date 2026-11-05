# Bug Fix Report: 50% Luminance Reduction on Food & Water Presentation Colors

- **Issue ID:** FIX-03
- **File Target:** `.planning/fixes/resource-color-contrast-darken.md`
- **Component:** Presentation / Visual Rendering Layer (`Common.hpp`, `Renderer.cpp`, `UI.cpp`, `test_rules.cpp`)
- **Status:** **VERIFIED & RESOLVED**

---

### [BUG-FIX]: Food and Water Presentation Contrast Darkening (50% Attenuation)

- **Defect Summary:** Food entities (`RGB(235, 55, 55)`) and Water entities (`RGB(250, 250, 255)`) were rendered at near-maximum channel brightness, causing numerous resource tiles to visually overpower and distract from active organism entities (`Male: RGB(50, 130, 240)`, `Female: RGB(240, 210, 40)`).
- **Root Cause Analysis:** Color presentation constants defined in `include/sim/Common.hpp` and rendered directly in `Renderer.DrawResources` and `UI.DrawSidebar` utilized high-luminance color hex values (`0xEB3737` and `0xFAFAFF`). When 500 initial resource instances populated the $100 \times 100$ grid, the high visual frequency and luminance created poor figure-ground contrast against organisms.
- **Target Files:**
  - `include/sim/Common.hpp` (Surgical modification of `COLOR_FOOD_RGB` and `COLOR_WATER_RGB`)
  - `src/app/Renderer.cpp` (Updated rendering colors for Food and Water tiles)
  - `src/app/UI.cpp` (Updated manual placement tool palette swatch colors)
  - `tests/test_rules.cpp` (Regression test asserting 50% darkened color thresholds)

---

### Regression Test Harness

```cpp
TEST_CASE(TC_RULE_10_Resource_Color_Contrast) {
    // 50% Darkened Food (R ~118, G ~28, B ~28), 50% Darkened Water (R ~125, G ~125, B ~128)
    // Darkened resources allow brighter organisms (Blue: 50,130,240 and Yellow: 240,210,40) to visually stand out.
    uint32_t food_r = (COLOR_FOOD_RGB >> 16) & 0xFF;
    uint32_t food_g = (COLOR_FOOD_RGB >> 8) & 0xFF;
    uint32_t food_b = COLOR_FOOD_RGB & 0xFF;
    CHECK_TRUE(food_r >= 110 && food_r <= 125);
    CHECK_TRUE(food_g >= 20 && food_g <= 35);
    CHECK_TRUE(food_b >= 20 && food_b <= 35);

    uint32_t water_r = (COLOR_WATER_RGB >> 16) & 0xFF;
    uint32_t water_g = (COLOR_WATER_RGB >> 8) & 0xFF;
    uint32_t water_b = COLOR_WATER_RGB & 0xFF;
    CHECK_TRUE(water_r >= 120 && water_r <= 130);
    CHECK_TRUE(water_g >= 120 && water_g <= 130);
    CHECK_TRUE(water_b >= 120 && water_b <= 130);
}
```

---

### Patch Diffs

#### 1. Core Color Constants (`include/sim/Common.hpp`)
```diff
--- a/include/sim/Common.hpp
+++ b/include/sim/Common.hpp
@@ -47,4 +47,4 @@
-// Resource Presentation Colors (Red for Food, White for Water for high visual
-// contrast)
-constexpr uint32_t COLOR_FOOD_RGB = 0xEB3737;
-constexpr uint32_t COLOR_WATER_RGB = 0xFAFAFF;
+// Resource Presentation Colors (50% Darkened: Deep Red for Food, Medium Slate/Gray for Water
+// to ensure brighter organism entities stand out on the playfield)
+constexpr uint32_t COLOR_FOOD_RGB = 0x761C1C;
+constexpr uint32_t COLOR_WATER_RGB = 0x7D7D80;
```

#### 2. Playfield Tile Rendering (`src/app/Renderer.cpp`)
```diff
--- a/src/app/Renderer.cpp
+++ b/src/app/Renderer.cpp
@@ -45,6 +45,6 @@
         if (res.type == sim::EntityType::FOOD) {
-            // Food: Vibrant Red for high contrast against blue/yellow organisms
-            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{235, 55, 55, 255});
+            // Food: 50% Darkened Red (118, 28, 28) for high contrast against brighter organisms
+            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{118, 28, 28, 255});
         } else if (res.type == sim::EntityType::WATER) {
-            // Water: Bright White for high contrast against dark playfield and colored organisms
-            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{250, 250, 255, 255});
+            // Water: 50% Darkened White/Gray (125, 125, 128) for high contrast against brighter organisms
+            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{125, 125, 128, 255});
         }
```

#### 3. Sidebar Tool Swatches (`src/app/UI.cpp`)
```diff
--- a/src/app/UI.cpp
+++ b/src/app/UI.cpp
@@ -114,2 +114,2 @@
-    draw_tool_item(1, "Food", "Unlimited", sim::ActiveTool::FOOD, Color{235, 55, 55, 255});
-    draw_tool_item(2, "Water", "Unlimited", sim::ActiveTool::WATER, Color{250, 250, 255, 255});
+    draw_tool_item(1, "Food", "Unlimited", sim::ActiveTool::FOOD, Color{118, 28, 28, 255});
+    draw_tool_item(2, "Water", "Unlimited", sim::ActiveTool::WATER, Color{125, 125, 128, 255});
```

---

### Verification Commands & Evidence

1. **Pre-patch Test Execution:**
   - Command: `cmake --build build --target sim_tests && ./build/sim_tests`
   - Result: Failed assertions in `TC_RULE_10_Resource_Color_Contrast` (channels confirmed at 235/55/55 and 250/250/255).
2. **Post-patch Test Execution:**
   - Command: `cmake --build build --target sim_tests && ./build/sim_tests`
   - Result:
     ```text
     [ RUN      ] TC_RULE_10_Resource_Color_Contrast
     [       OK ] TC_RULE_10_Resource_Color_Contrast
     =======================================
      Test Summary: 50 passed, 0 failed (266 total assertions)
     =======================================
     ```
3. **Application Build:**
   - Command: `cmake --build build --target organism_sim`
   - Result: Built target `organism_sim` with 0 warnings.

---

### Side-Effect Audit
- [x] Public API contracts preserved.
- [x] Simulation logic, pathfinding, and metabolism unaffected (pure visual tuning).
- [x] No extraneous formatting or refactoring changes included.
- [x] All 50 test cases pass cleanly under AddressSanitizer and UndefinedBehaviorSanitizer.

---

*Lifecycle Citation:*
Following the Engineering Lifecycle Workflow:
- To initialize version control, check hygiene, and publish the repository to GitHub, invoke **`github-repo-publisher`**.

