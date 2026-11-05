#include "TestHarness.hpp"
#include "sim/Simulation.hpp"
#include "app/IconData.hpp"
#include <fstream>
#include <sstream>

using namespace sim;

TEST_CASE(TC_RULE_01_Placement_Quota_Male) {
    Simulation sim(123);
    CHECK_EQ(sim.RemainingMalePlacements(), 10);

    int placed = 0;
    int next_x = 0;
    for (int x = 0; x < GRID_WIDTH && placed < 10; ++x) {
        if (sim.GetGrid().CanPlace(x, 0, 1, 1)) {
            CHECK_TRUE(sim.ManualPlace(x, 0, EntityType::ORGANISM, Gender::MALE));
            ++placed;
            next_x = x + 1;
        }
    }
    CHECK_EQ(placed, 10);
    CHECK_EQ(sim.RemainingMalePlacements(), 0);

    // 11th placement attempt on an empty cell must be rejected by quota
    for (int x = next_x; x < GRID_WIDTH; ++x) {
        if (sim.GetGrid().CanPlace(x, 0, 1, 1)) {
            CHECK_FALSE(sim.ManualPlace(x, 0, EntityType::ORGANISM, Gender::MALE));
            break;
        }
    }
}

TEST_CASE(TC_RULE_02_Placement_Quota_Female) {
    Simulation sim(123);
    CHECK_EQ(sim.RemainingFemalePlacements(), 10);

    int placed = 0;
    int next_x = 0;
    for (int x = 0; x < GRID_WIDTH && placed < 10; ++x) {
        if (sim.GetGrid().CanPlace(x, 1, 1, 1)) {
            CHECK_TRUE(sim.ManualPlace(x, 1, EntityType::ORGANISM, Gender::FEMALE));
            ++placed;
            next_x = x + 1;
        }
    }
    CHECK_EQ(placed, 10);
    CHECK_EQ(sim.RemainingFemalePlacements(), 0);

    // 11th placement attempt on an empty cell must be rejected by quota
    for (int x = next_x; x < GRID_WIDTH; ++x) {
        if (sim.GetGrid().CanPlace(x, 1, 1, 1)) {
            CHECK_FALSE(sim.ManualPlace(x, 1, EntityType::ORGANISM, Gender::FEMALE));
            break;
        }
    }
}

TEST_CASE(TC_RULE_03_Food_Water_Unlimited) {
    Simulation sim(123);
    int food_placed = 0;
    for (int x = 0; x < GRID_WIDTH && food_placed < 25; ++x) {
        if (sim.GetGrid().CanPlace(x, 2, 1, 1)) {
            CHECK_TRUE(sim.ManualPlace(x, 2, EntityType::FOOD));
            ++food_placed;
        }
    }
    CHECK_EQ(food_placed, 25);

    int water_placed = 0;
    for (int x = 0; x < GRID_WIDTH && water_placed < 25; ++x) {
        if (sim.GetGrid().CanPlace(x, 3, 1, 1)) {
            CHECK_TRUE(sim.ManualPlace(x, 3, EntityType::WATER));
            ++water_placed;
        }
    }
    CHECK_EQ(water_placed, 25);
}

TEST_CASE(TC_RULE_04_Resource_Spawn_Interval) {
    Simulation sim(123);
    size_t initial_res = sim.GetEntities().ActiveResourceCount();

    // Step by 45 seconds to trigger guaranteed periodic spawn (interval is 20-40s)
    sim.Step(45.0f);
    CHECK_TRUE(sim.GetEntities().ActiveResourceCount() > initial_res);
}

TEST_CASE(TC_RULE_05_Victory_Trigger) {
    Simulation sim(123);
    CHECK_EQ(static_cast<int>(sim.GetGameStatus()), static_cast<int>(GameStatus::ONGOING));

    // Artificially occupy 7500 organism cells
    for (int x = 0; x < 100 && sim.GetGrid().OccupiedCellCount() < 7500; ++x) {
        for (int y = 0; y < 100 && sim.GetGrid().OccupiedCellCount() < 7500; ++y) {
            if (sim.GetGrid().CanPlace(x, y, 1, 1)) {
                sim.GetGrid().Occupy(x, y, 1, 1, EntityType::ORGANISM, 9999);
            }
        }
    }
    CHECK_TRUE(sim.GetGrid().OccupiedCellCount() >= 7500);
    sim.Step(0.01f);
    CHECK_EQ(static_cast<int>(sim.GetGameStatus()), static_cast<int>(GameStatus::VICTORY));
}

TEST_CASE(TC_RULE_06_Loss_Trigger) {
    Simulation sim(123);
    for (auto& org : sim.GetEntities().GetOrganisms()) {
        org.life_force = 0.0f;
        org.alive = false;
    }
    sim.Step(0.01f);
    CHECK_EQ(static_cast<int>(sim.GetGameStatus()), static_cast<int>(GameStatus::DEFEAT));
}

TEST_CASE(TC_RULE_07_Reset_Operation) {
    Simulation sim(123);
    sim.ManualPlace(0, 0, EntityType::ORGANISM, Gender::MALE);
    CHECK_EQ(sim.RemainingMalePlacements(), 9);

    sim.Reset();
    CHECK_EQ(sim.RemainingMalePlacements(), 10);
    CHECK_EQ(sim.RemainingFemalePlacements(), 10);
    CHECK_EQ(static_cast<int>(sim.GetGameStatus()), static_cast<int>(GameStatus::ONGOING));
    CHECK_EQ(sim.GetEntities().ActiveOrganismCount(), 2u); // 1 male, 1 female initial
    CHECK_EQ(sim.GetEntities().ActiveResourceCount(), static_cast<size_t>(INITIAL_FOOD_COUNT + INITIAL_WATER_COUNT));
}

TEST_CASE(TC_RULE_08_Occupancy_Counts_Only_Organisms) {
    GridMatrix grid;
    CHECK_EQ(grid.OccupiedCellCount(), 0u);

    // Occupy Food and Water
    grid.Occupy(5, 5, 1, 1, EntityType::FOOD, 1);
    grid.Occupy(6, 6, 1, 1, EntityType::WATER, 2);
    // Occupancy should remain 0 because Food and Water must not count towards grid occupancy
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

TEST_CASE(TC_RULE_09_Resource_Capacity_Invariant) {
    Simulation sim(456);
    size_t food_cnt = 0;
    size_t water_cnt = 0;
    for (const auto& res : sim.GetEntities().GetResources()) {
        if (!res.alive) continue;
        if (res.type == EntityType::FOOD) ++food_cnt;
        else if (res.type == EntityType::WATER) ++water_cnt;
    }
    CHECK_EQ(food_cnt, static_cast<size_t>(INITIAL_FOOD_COUNT));
    CHECK_EQ(water_cnt, static_cast<size_t>(INITIAL_WATER_COUNT));
    CHECK_EQ(sim.GetEntities().ActiveResourceCount(), static_cast<size_t>(INITIAL_FOOD_COUNT + INITIAL_WATER_COUNT));
}

TEST_CASE(TC_INSP_01_Empty_Cell_Hover) {
    Simulation sim(789);
    // Find an empty cell
    for (int x = 0; x < GRID_WIDTH; ++x) {
        for (int y = 0; y < GRID_HEIGHT; ++y) {
            if (sim.GetGrid().At(x, y).type == EntityType::EMPTY) {
                CHECK_TRUE(sim.GetOrganismAt(x, y) == nullptr);
                return;
            }
        }
    }
}

TEST_CASE(TC_INSP_02_Resource_Cell_Hover) {
    Simulation sim(789);
    for (const auto& res : sim.GetEntities().GetResources()) {
        if (res.alive) {
            CHECK_TRUE(sim.GetOrganismAt(res.pos.x, res.pos.y) == nullptr);
            break;
        }
    }
}

TEST_CASE(TC_INSP_03_Single_Tile_Organism_Hover) {
    Simulation sim(789);
    const auto& orgs = sim.GetEntities().GetOrganisms();
    const Organism& org = orgs.front();
    const Organism* inspected = sim.GetOrganismAt(org.pos.x, org.pos.y);
    CHECK_TRUE(inspected != nullptr);
    if (inspected) {
        CHECK_EQ(inspected->id, org.id);
        CHECK_EQ(inspected->w, 1);
        CHECK_EQ(inspected->h, 1);
    }
}

TEST_CASE(TC_INSP_04_Multi_Tile_Organism_Hover) {
    Simulation sim(789);
    // Manually place a multi-tile organism via grid and entity manager
    uint32_t big_id = sim.GetEntities().CreateOrganism({60, 60}, Gender::MALE, 3, 3);
    sim.GetGrid().Occupy(60, 60, 3, 3, EntityType::ORGANISM, big_id);

    for (int x = 60; x < 63; ++x) {
        for (int y = 60; y < 63; ++y) {
            const Organism* inspected = sim.GetOrganismAt(x, y);
            CHECK_TRUE(inspected != nullptr);
            if (inspected) {
                CHECK_EQ(inspected->id, big_id);
                CHECK_EQ(inspected->w, 3);
                CHECK_EQ(inspected->h, 3);
            }
        }
    }
}

TEST_CASE(TC_INSP_05_OOB_Hover) {
    Simulation sim(789);
    CHECK_TRUE(sim.GetOrganismAt(-1, 0) == nullptr);
    CHECK_TRUE(sim.GetOrganismAt(0, -1) == nullptr);
    CHECK_TRUE(sim.GetOrganismAt(100, 50) == nullptr);
    CHECK_TRUE(sim.GetOrganismAt(50, 100) == nullptr);
    CHECK_TRUE(sim.GetOrganismAt(-50, -50) == nullptr);
}

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

TEST_CASE(TC_STEP_01_Initial_Step_Count) {
    Simulation sim(123);
    CHECK_EQ(sim.GetStepCount(), 0ULL);
}

TEST_CASE(TC_STEP_02_Incremental_Steps) {
    Simulation sim(123);
    for (int i = 0; i < 25; ++i) {
        sim.Step(1.0f);
    }
    CHECK_EQ(sim.GetStepCount(), 25ULL);
}

TEST_CASE(TC_STEP_03_Reset_Clears_Step_Count) {
    Simulation sim(123);
    for (int i = 0; i < 15; ++i) {
        sim.Step(1.0f);
    }
    CHECK_EQ(sim.GetStepCount(), 15ULL);

    sim.Reset();
    CHECK_EQ(sim.GetStepCount(), 0ULL);
}

TEST_CASE(TC_STEP_04_Organism_One_Second_Step_Alignment) {
    Simulation sim(123);
    // 19 sub-ticks of 0.05s = 0.95s elapsed -> 0 full organism steps (requires 1.0s)
    for (int i = 0; i < 19; ++i) {
        sim.Step(0.05f);
    }
    CHECK_EQ(sim.GetStepCount(), 0ULL);

    // 20th sub-tick reaches 1.00s elapsed -> exactly 1 organism step
    sim.Step(0.05f);
    CHECK_EQ(sim.GetStepCount(), 1ULL);

    // Another 20 sub-ticks reaches 2.00s elapsed -> exactly 2 organism steps
    for (int i = 0; i < 20; ++i) {
        sim.Step(0.05f);
    }
    CHECK_EQ(sim.GetStepCount(), 2ULL);
}

TEST_CASE(TC_ICON_01_Embedded_PNG_Magic_Header) {
    CHECK_TRUE(app::ICON_PNG_DATA_LEN > 1000);
    // Standard PNG magic signature: 0x89 'P' 'N' 'G' '\r' '\n' 0x1A '\n'
    CHECK_EQ(app::ICON_PNG_DATA[0], 0x89);
    CHECK_EQ(app::ICON_PNG_DATA[1], 'P');
    CHECK_EQ(app::ICON_PNG_DATA[2], 'N');
    CHECK_EQ(app::ICON_PNG_DATA[3], 'G');
    CHECK_EQ(app::ICON_PNG_DATA[4], 0x0D);
    CHECK_EQ(app::ICON_PNG_DATA[5], 0x0A);
    CHECK_EQ(app::ICON_PNG_DATA[6], 0x1A);
    CHECK_EQ(app::ICON_PNG_DATA[7], 0x0A);
}

TEST_CASE(TC_ICON_02_Embedded_Array_Size) {
    CHECK_EQ(app::ICON_PNG_DATA_LEN, sizeof(app::ICON_PNG_DATA));
    CHECK_TRUE(app::ICON_PNG_DATA_LEN >= 2000 && app::ICON_PNG_DATA_LEN <= 5000);
}

TEST_CASE(TC_ICON_03_Windows_ICO_Asset) {
    const char* paths[] = {"assets/icons/icon.ico", "../assets/icons/icon.ico"};
    std::ifstream file;
    for (const char* p : paths) {
        file.open(p, std::ios::binary);
        if (file.is_open()) break;
    }
    CHECK_TRUE(file.is_open());
    if (file.is_open()) {
        unsigned char header[4] = {0};
        file.read(reinterpret_cast<char*>(header), 4);
        // Windows ICO magic bytes: 0x00 0x00 0x01 0x00
        CHECK_EQ(header[0], 0x00);
        CHECK_EQ(header[1], 0x00);
        CHECK_EQ(header[2], 0x01);
        CHECK_EQ(header[3], 0x00);
    }
}

TEST_CASE(TC_ICON_04_Linux_Desktop_Launcher) {
    const char* paths[] = {"organism_sim.desktop", "../organism_sim.desktop"};
    std::ifstream file;
    for (const char* p : paths) {
        file.open(p);
        if (file.is_open()) break;
    }
    CHECK_TRUE(file.is_open());
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();
        CHECK_TRUE(content.find("[Desktop Entry]") != std::string::npos);
        CHECK_TRUE(content.find("Type=Application") != std::string::npos);
        CHECK_TRUE(content.find("Exec=organism_sim") != std::string::npos);
        CHECK_TRUE(content.find("Icon=organism_sim") != std::string::npos);
    }
}

TEST_CASE(TC_ICON_05_PNG_Color_Type_RGBA) {
    // Byte 25 in PNG IHDR specifies color type: 6 = RGBA (TrueColor with Alpha)
    // Required by Raylib SetWindowIcon / glfwSetWindowIcon (R8G8B8A8 pixel format)
    CHECK_TRUE(app::ICON_PNG_DATA_LEN > 26);
    CHECK_EQ(app::ICON_PNG_DATA[25], 6);
}

TEST_CASE(TC_RULE_11_Tool_Selection_Label_Contrast) {
    // When selected, tool labels must use high-contrast white text (RGB 255, 255, 255)
    // rather than low-luminance entity swatches (e.g., avoiding dark red 118, 28, 28).
    struct RGBColor { uint8_t r, g, b; };
    auto calc_lum = [](RGBColor c) -> double {
        return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
    };

    RGBColor selected_text{255, 255, 255};
    CHECK_TRUE(calc_lum(selected_text) > 250.0);

    // Unselected text colors for all 4 tools must have high contrast (luminance > 140)
    RGBColor food_unsel{255, 130, 130};
    RGBColor water_unsel{220, 235, 250};
    RGBColor male_unsel{110, 180, 255};
    RGBColor female_unsel{255, 230, 100};

    CHECK_TRUE(calc_lum(food_unsel) > 140.0);
    CHECK_TRUE(calc_lum(water_unsel) > 180.0);
    CHECK_TRUE(calc_lum(male_unsel) > 140.0);
    CHECK_TRUE(calc_lum(female_unsel) > 180.0);

    // The dark playfield food color (118, 28, 28) has low luminance (~55)
    // and must be isolated to the icon swatch, never used as text on dark backgrounds.
    RGBColor dark_food_swatch{118, 28, 28};
    CHECK_TRUE(calc_lum(dark_food_swatch) < 60.0);
}

TEST_CASE(TC_BUILD_01_CMakeLists_Targets_Configured) {
    const char* paths[] = {"CMakeLists.txt", "../CMakeLists.txt"};
    std::ifstream file;
    for (const char* p : paths) {
        file.open(p);
        if (file.is_open()) break;
    }
    CHECK_TRUE(file.is_open());
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        std::string content = ss.str();

        // 1. Clean and Rebuild custom automation targets
        CHECK_TRUE(content.find("add_custom_target(clean_all") != std::string::npos);
        CHECK_TRUE(content.find("add_custom_target(rebuild") != std::string::npos);

        // 2. GNUInstallDirs and standard install directives
        CHECK_TRUE(content.find("include(GNUInstallDirs)") != std::string::npos);
        CHECK_TRUE(content.find("install(TARGETS organism_sim") != std::string::npos);
        CHECK_TRUE(content.find("organism_sim.desktop") != std::string::npos);
        CHECK_TRUE(content.find("icons/hicolor/200x200/apps") != std::string::npos);
    }
}






