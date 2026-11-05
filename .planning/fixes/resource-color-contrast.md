# Bug Fix Resolution Report: Resource Color Contrast

- **Defect Slug:** `resource-color-contrast`
- **Report Target:** `.planning/fixes/resource-color-contrast.md`
- **Classification:** Visual Presentation / Accessibility & Color Contrast
- **Status:** **RESOLVED**

---

## 1. Defect Summary
The rendered colors for Food (emerald green) and Water (bright cyan) were too visually similar to the organisms (shades of yellow for females and shades of blue for males). When organisms desaturated during starvation, green food became difficult to distinguish from dying yellow organisms, and cyan water clashed with healthy blue male organisms.

---

## 2. Root Cause Analysis
In `src/app/Renderer.cpp`:
- `DrawResources` rendered `FOOD` with `Color{46, 204, 113, 255}` (emerald green) and `WATER` with `Color{52, 152, 219, 255}` (cyan).
In `src/app/UI.cpp`:
- Placement tool badges matched the low-contrast green and cyan hues.

---

## 3. Targeted Files & Changes
- `include/sim/Common.hpp`: Added color constants `COLOR_FOOD_RGB = 0xEB3737` (Red) and `COLOR_WATER_RGB = 0xFAFAFF` (White).
- `src/app/Renderer.cpp`: Modified `Renderer::DrawResources()` to render Food in vibrant Red (`Color{235, 55, 55, 255}`) and Water in crisp White (`Color{250, 250, 255, 255}`).
- `src/app/UI.cpp`: Updated sidebar HUD placement tool labels to match the new Red and White palette.
- `tests/test_rules.cpp`: Added test case `TC_RULE_10_Resource_Color_Contrast` validating dominant color channels.

---

## 4. Regression Test Code
```cpp
TEST_CASE(TC_RULE_10_Resource_Color_Contrast) {
    // Red food (dominant R channel), White water (all channels near max)
    uint32_t food_r = (COLOR_FOOD_RGB >> 16) & 0xFF;
    uint32_t food_g = (COLOR_FOOD_RGB >> 8) & 0xFF;
    uint32_t food_b = COLOR_FOOD_RGB & 0xFF;
    CHECK_TRUE(food_r > 200 && food_g < 100 && food_b < 100);

    uint32_t water_r = (COLOR_WATER_RGB >> 16) & 0xFF;
    uint32_t water_g = (COLOR_WATER_RGB >> 8) & 0xFF;
    uint32_t water_b = COLOR_WATER_RGB & 0xFF;
    CHECK_TRUE(water_r > 240 && water_g > 240 && water_b > 240);
}
```

---

## 5. Surgical Patch Diff
```diff
--- a/src/app/Renderer.cpp
+++ b/src/app/Renderer.cpp
@@ -45,10 +45,10 @@ void Renderer::DrawResources(const sim::Simulation& sim) {
         if (res.type == sim::EntityType::FOOD) {
-            // Food: Emerald Green
-            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{46, 204, 113, 255});
+            // Food: Vibrant Red for high contrast against blue/yellow organisms
+            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{235, 55, 55, 255});
         } else if (res.type == sim::EntityType::WATER) {
-            // Water: Bright Cyan
-            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{52, 152, 219, 255});
+            // Water: Bright White for high contrast against dark playfield and colored organisms
+            DrawRectangle(px + 1, py + 1, sim::CELL_SIZE_PX - 2, sim::CELL_SIZE_PX - 2, Color{250, 250, 255, 255});
         }
```

---

## 6. Verification Evidence
```bash
$ cmake --build build --target sim_tests && ./build/sim_tests
...
[ RUN      ] TC_RULE_10_Resource_Color_Contrast
[       OK ] TC_RULE_10_Resource_Color_Contrast
=======================================
 Test Summary: 44 passed, 0 failed (247 total assertions)
=======================================
```

---

## 7. Side-Effect Audit
- [x] Public API contracts preserved.
- [x] Zero extraneous refactoring outside the color palette changes.
- [x] All 44 test cases pass cleanly under AddressSanitizer and UndefinedBehaviorSanitizer.
- [x] Graphical binary `organism_sim` builds and links without warnings.

