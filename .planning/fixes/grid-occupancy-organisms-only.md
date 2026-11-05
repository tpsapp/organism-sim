# Bug Fix Resolution Report: Grid Occupancy Organism Filter

- **Defect Slug:** `grid-occupancy-organisms-only`
- **Report Target:** `.planning/fixes/grid-occupancy-organisms-only.md`
- **Classification:** Logical Metric Distortion / Calculation Defect
- **Status:** **RESOLVED**

---

## 1. Defect Summary
The grid occupancy metric and victory coverage evaluation were incorrectly tallying static resource cells (`FOOD` and `WATER`) alongside organisms. As a result, placing resources artificially inflated the grid occupancy count and percentage towards the 75% victory condition ($7,500$ cells), rather than measuring only organism multi-tile footprint coverage.

---

## 2. Root Cause Analysis
In `src/sim/GridMatrix.cpp`:
- `GridMatrix::Occupy()` incremented `occupied_count_` for any transition from `EntityType::EMPTY` regardless of whether `type` was `FOOD`, `WATER`, or `ORGANISM`.
- `GridMatrix::Vacate()` decremented `occupied_count_` for any non-empty cell cleared, rather than restricting the count to `EntityType::ORGANISM`.

Because initial game world initialization seeds 20 Food and 20 Water items, `OccupiedCellCount()` started at 42 cells instead of the 2 initial organism cells ($1 \times 1$ male, $1 \times 1$ female).

---

## 3. Targeted Files & Changes
- `tests/test_rules.cpp`: Added reproduction test case `TC_RULE_08_Occupancy_Counts_Only_Organisms` and updated `TC_RULE_05_Victory_Trigger` to verify organism cell counting.
- `src/sim/GridMatrix.cpp`: Modified `Occupy()` and `Vacate()` to exclusively increment/decrement `occupied_count_` when `EntityType::ORGANISM` is placed or cleared.

---

## 4. Regression Test Code
```cpp
TEST_CASE(TC_RULE_08_Occupancy_Counts_Only_Organisms) {
    GridMatrix grid;
    CHECK_EQ(grid.OccupiedCellCount(), 0u);

    // Occupy Food and Water
    grid.Occupy(5, 5, 1, 1, EntityType::FOOD, 1);
    grid.Occupy(6, 6, 1, 1, EntityType::WATER, 2);
    // Occupancy must remain 0 because Food and Water do not count
    CHECK_EQ(grid.OccupiedCellCount(), 0u);

    // Occupy 1 Organism of size 2x2
    grid.Occupy(10, 10, 2, 2, EntityType::ORGANISM, 3);
    CHECK_EQ(grid.OccupiedCellCount(), 4u);

    // Vacate Food
    grid.Vacate(5, 5, 1, 1);
    CHECK_EQ(grid.OccupiedCellCount(), 4u);

    // Vacate Organism
    grid.Vacate(10, 10, 2, 2);
    CHECK_EQ(grid.OccupiedCellCount(), 0u);
}
```

---

## 5. Surgical Patch Diff
```diff
--- a/src/sim/GridMatrix.cpp
+++ b/src/sim/GridMatrix.cpp
@@ -42,7 +42,7 @@ void GridMatrix::Occupy(int x, int y, int w, int h, EntityType type, uint32_t en
     for (int cx = x; cx < x + w; ++cx) {
         for (int cy = y; cy < y + h; ++cy) {
-            if (cells_[cx][cy].type == EntityType::EMPTY) {
+            if (type == EntityType::ORGANISM && cells_[cx][cy].type != EntityType::ORGANISM) {
                 ++occupied_count_;
             }
             cells_[cx][cy] = Cell{type, entity_id};
@@ -58,7 +58,7 @@ void GridMatrix::Vacate(int x, int y, int w, int h) {
     for (int cx = x; cx < max_x; ++cx) {
         for (int cy = y; cy < max_y; ++cy) {
-            if (cells_[cx][cy].type != EntityType::EMPTY) {
+            if (cells_[cx][cy].type == EntityType::ORGANISM) {
                 if (occupied_count_ > 0) {
                     --occupied_count_;
                 }
```

---

## 6. Verification Evidence
```bash
$ cmake --build build --target sim_tests && ./build/sim_tests
...
[ RUN      ] TC_RULE_08_Occupancy_Counts_Only_Organisms
[       OK ] TC_RULE_08_Occupancy_Counts_Only_Organisms
=======================================
 Test Summary: 37 passed, 0 failed (194 total assertions)
=======================================
```

---

## 7. Side-Effect Audit
- [x] Public API contracts preserved (`GridMatrix::OccupiedCellCount()` remains valid and now accurately reports organism footprint cells).
- [x] Zero extraneous refactoring or formatting modifications outside defective lines.
- [x] All 37 headless test cases pass under AddressSanitizer and UndefinedBehaviorSanitizer.

