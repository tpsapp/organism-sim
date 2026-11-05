#include "TestHarness.hpp"
#include "sim/GridMatrix.hpp"

using namespace sim;

TEST_CASE(TC_GRID_01_HappyPath_1x1) {
    GridMatrix grid;
    CHECK_TRUE(grid.CanPlace(50, 50, 1, 1));
    grid.Occupy(50, 50, 1, 1, EntityType::ORGANISM, 101);
    CHECK_EQ(static_cast<int>(grid.At(50, 50).type), static_cast<int>(EntityType::ORGANISM));
    CHECK_EQ(grid.At(50, 50).entity_id, 101u);
    CHECK_EQ(grid.OccupiedCellCount(), 1u);
}

TEST_CASE(TC_GRID_02_HappyPath_4x4) {
    GridMatrix grid;
    CHECK_TRUE(grid.CanPlace(10, 10, 4, 4));
    grid.Occupy(10, 10, 4, 4, EntityType::ORGANISM, 102);
    for (int x = 10; x < 14; ++x) {
        for (int y = 10; y < 14; ++y) {
            CHECK_EQ(static_cast<int>(grid.At(x, y).type), static_cast<int>(EntityType::ORGANISM));
            CHECK_EQ(grid.At(x, y).entity_id, 102u);
        }
    }
    CHECK_EQ(grid.OccupiedCellCount(), 16u);
}

TEST_CASE(TC_GRID_03_Boundary_Corner_96_96) {
    GridMatrix grid;
    CHECK_TRUE(grid.CanPlace(96, 96, 4, 4));
    grid.Occupy(96, 96, 4, 4, EntityType::ORGANISM, 103);
    CHECK_EQ(grid.OccupiedCellCount(), 16u);
}

TEST_CASE(TC_GRID_04_Boundary_OOB_X) {
    GridMatrix grid;
    CHECK_FALSE(grid.CanPlace(97, 10, 4, 4));
    CHECK_EQ(grid.OccupiedCellCount(), 0u);
}

TEST_CASE(TC_GRID_05_Boundary_OOB_Y) {
    GridMatrix grid;
    CHECK_FALSE(grid.CanPlace(10, 97, 4, 4));
    CHECK_EQ(grid.OccupiedCellCount(), 0u);
}

TEST_CASE(TC_GRID_06_Boundary_Negative) {
    GridMatrix grid;
    CHECK_FALSE(grid.CanPlace(-1, 0, 1, 1));
    CHECK_FALSE(grid.CanPlace(0, -1, 1, 1));
    CHECK_FALSE(grid.CanPlace(-5, -5, 4, 4));
    CHECK_EQ(grid.OccupiedCellCount(), 0u);
}

TEST_CASE(TC_GRID_07_Collision_Overlap) {
    GridMatrix grid;
    grid.Occupy(20, 20, 2, 2, EntityType::ORGANISM, 201);
    CHECK_FALSE(grid.CanPlace(21, 21, 2, 2));
    CHECK_FALSE(grid.CanPlace(19, 19, 2, 2));
    CHECK_EQ(grid.OccupiedCellCount(), 4u);
}

TEST_CASE(TC_GRID_08_InPlace_Vacate) {
    GridMatrix grid;
    grid.Occupy(30, 30, 3, 3, EntityType::ORGANISM, 301);
    CHECK_EQ(grid.OccupiedCellCount(), 9u);
    grid.Vacate(30, 30, 3, 3);
    CHECK_EQ(grid.OccupiedCellCount(), 0u);
    for (int x = 30; x < 33; ++x) {
        for (int y = 30; y < 33; ++y) {
            CHECK_EQ(static_cast<int>(grid.At(x, y).type), static_cast<int>(EntityType::EMPTY));
            CHECK_EQ(grid.At(x, y).entity_id, 0u);
        }
    }
}

TEST_CASE(TC_GRID_09_Growth_Expansion) {
    GridMatrix grid;
    grid.Occupy(40, 40, 2, 2, EntityType::ORGANISM, 401);
    CHECK_EQ(grid.OccupiedCellCount(), 4u);
    CHECK_TRUE(grid.CanExpand(40, 40, 2, 2, 3, 2, 401));
    // Re-occupy expanded footprint
    grid.Occupy(40, 40, 3, 2, EntityType::ORGANISM, 401);
    CHECK_EQ(grid.OccupiedCellCount(), 6u);
}

TEST_CASE(TC_GRID_10_Growth_Collision) {
    GridMatrix grid;
    grid.Occupy(40, 40, 2, 2, EntityType::ORGANISM, 401);
    grid.Occupy(42, 40, 1, 1, EntityType::FOOD, 501);
    CHECK_FALSE(grid.CanExpand(40, 40, 2, 2, 3, 2, 401));
}

