#include "TestHarness.hpp"
#include "sim/AIEngine.hpp"

using namespace sim;

TEST_CASE(TC_AI_01_Chebyshev_Distance_Boxes) {
    Vector2i a{10, 10};
    Vector2i b{14, 10};
    int dist = AIEngine::ChebyshevDistance(a, 2, 2, b, 1, 1);
    CHECK_EQ(dist, 3);
}

TEST_CASE(TC_AI_02_Multi_Tile_Adjacency) {
    Vector2i a{10, 10};
    Vector2i b{12, 11};
    CHECK_TRUE(AIEngine::AreAdjacent(a, 2, 2, b, 1, 1));
}

TEST_CASE(TC_AI_03_Priority_Need_Over_Mate) {
    GridMatrix grid;
    EntityManager em;
    uint32_t mid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 40, 10);
    uint32_t fid = em.CreateOrganism({11, 10}, Gender::FEMALE, 1, 1, 40, 12);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, mid);
    grid.Occupy(11, 10, 1, 1, EntityType::ORGANISM, fid);

    Organism* male = em.GetOrganism(mid);
    male->thirst = 2.0f; // Critical
    male->age = 3;
    male->hunger = 10.0f;

    Organism* female = em.GetOrganism(fid);
    female->age = 3;
    female->hunger = 10.0f;
    female->thirst = 10.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*male, em, grid, 0.1f, rng);

    CHECK_EQ(static_cast<int>(male->current_state), static_cast<int>(FSMState::CRITICAL_NEED));
    CHECK_FALSE(female->gestating);
}

TEST_CASE(TC_AI_04_Reproduction_Compatible) {
    GridMatrix grid;
    EntityManager em;
    uint32_t mid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 40, 10);
    uint32_t fid = em.CreateOrganism({11, 10}, Gender::FEMALE, 1, 1, 40, 12); // diff = 2, 2 % 10 = 2 < 7
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, mid);
    grid.Occupy(11, 10, 1, 1, EntityType::ORGANISM, fid);

    Organism* male = em.GetOrganism(mid);
    Organism* female = em.GetOrganism(fid);
    male->age = 3;
    female->age = 3;
    male->hunger = 8.0f;
    female->hunger = 8.0f;
    male->thirst = 8.0f;
    female->thirst = 8.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*male, em, grid, 0.1f, rng);

    CHECK_TRUE(female->gestating);
    CHECK_NEAR(female->gestation_timer, 90.0f, 0.01);
}

TEST_CASE(TC_AI_05_Incompatible_Scent) {
    GridMatrix grid;
    EntityManager em;
    uint32_t mid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 40, 10);
    uint32_t fid = em.CreateOrganism({11, 10}, Gender::FEMALE, 1, 1, 40, 18); // diff = 8, 8 % 10 = 8 >= 7
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, mid);
    grid.Occupy(11, 10, 1, 1, EntityType::ORGANISM, fid);

    Organism* male = em.GetOrganism(mid);
    Organism* female = em.GetOrganism(fid);
    male->age = 3;
    female->age = 3;
    male->hunger = 8.0f;
    female->hunger = 8.0f;
    male->thirst = 8.0f;
    female->thirst = 8.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*male, em, grid, 0.1f, rng);

    CHECK_FALSE(female->gestating);
}

TEST_CASE(TC_AI_06_Gestation_And_Offspring_Birth) {
    GridMatrix grid;
    EntityManager em;
    uint32_t fid = em.CreateOrganism({50, 50}, Gender::FEMALE, 1, 1, 40, 100);
    grid.Occupy(50, 50, 1, 1, EntityType::ORGANISM, fid);

    Organism* female = em.GetOrganism(fid);
    female->gestating = true;
    female->gestation_timer = 0.5f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(fid, em, grid, 1.0f, rng);

    female = em.GetOrganism(fid);
    CHECK_TRUE(female != nullptr);
    CHECK_FALSE(female->gestating);
    CHECK_EQ(em.ActiveOrganismCount(), 2u);
}

TEST_CASE(TC_AI_07_Gestation_Blocked) {
    GridMatrix grid;
    EntityManager em;
    uint32_t fid = em.CreateOrganism({50, 50}, Gender::FEMALE, 1, 1, 40, 100);
    grid.Occupy(50, 50, 1, 1, EntityType::ORGANISM, fid);

    // Block all 8 surrounding cells with food
    static const Vector2i kOffsets[8] = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0},
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    };
    for (const auto& off : kOffsets) {
        grid.Occupy(50 + off.x, 50 + off.y, 1, 1, EntityType::FOOD, 999);
    }

    Organism* female = em.GetOrganism(fid);
    female->gestating = true;
    female->gestation_timer = 0.5f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*female, em, grid, 1.0f, rng);

    // Birth blocked, remains gestating at 0.0s, no crash
    CHECK_TRUE(female->gestating);
    CHECK_NEAR(female->gestation_timer, 0.0f, 0.01);
    CHECK_EQ(em.ActiveOrganismCount(), 1u);
}

TEST_CASE(TC_AI_08_Wander_Fallback) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 40, 10);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, oid);

    Organism* org = em.GetOrganism(oid);
    org->step_cooldown = 0.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*org, em, grid, 1.0f, rng);

    CHECK_EQ(static_cast<int>(org->current_state), static_cast<int>(FSMState::WANDER));
}

TEST_CASE(TC_AI_09_Pathfinding_Unreachable_Target_Fallback) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 40, 10);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, oid);

    Organism* org = em.GetOrganism(oid);
    org->thirst = 2.0f; // Critical need for water
    org->step_cooldown = 0.0f;

    // Place Water A at (13, 10) (closer, distance 3), but enclose it completely
    uint32_t w_blocked = em.CreateResource({13, 10}, EntityType::WATER);
    grid.Occupy(13, 10, 1, 1, EntityType::WATER, w_blocked);

    static const Vector2i kWallOffsets[8] = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0},
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    };
    for (const auto& off : kWallOffsets) {
        grid.Occupy(13 + off.x, 10 + off.y, 1, 1, EntityType::ORGANISM, 9990);
    }

    // Place Water B at (10, 15) (distance 5, completely open and accessible)
    uint32_t w_open = em.CreateResource({10, 15}, EntityType::WATER);
    grid.Occupy(10, 15, 1, 1, EntityType::WATER, w_open);

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(oid, em, grid, 1.0f, rng);

    org = em.GetOrganism(oid);
    // The organism should step towards the reachable Water B (moving towards y=15, so y=11),
    // and NOT towards the blocked Water A (x=13).
    CHECK_EQ(org->pos.y, 11);
    CHECK_EQ(org->pos.x, 10);
}

TEST_CASE(TC_GEN_01_Biparental_Inheritance) {
    GridMatrix grid;
    EntityManager em;
    // Father with high traits: sight=6, smell=10, scent=100
    uint32_t mid = em.CreateOrganism({20, 20}, Gender::MALE, 1, 1, 40, 100);
    // Mother with low traits: sight=2, smell=4, scent=104 (diff=4, 4%10=4 < 7 compatible)
    uint32_t fid = em.CreateOrganism({21, 20}, Gender::FEMALE, 1, 1, 40, 104);

    Organism* male = em.GetOrganism(mid);
    male->sight_range = 6;
    male->smell_range = 10;
    male->age = 3;
    male->hunger = 10.0f;
    male->thirst = 10.0f;
    grid.Occupy(20, 20, 1, 1, EntityType::ORGANISM, mid);

    Organism* female = em.GetOrganism(fid);
    female->sight_range = 2;
    female->smell_range = 4;
    female->age = 3;
    female->hunger = 10.0f;
    female->thirst = 10.0f;
    grid.Occupy(21, 20, 1, 1, EntityType::ORGANISM, fid);

    AIEngine ai;
    std::mt19937 rng(12345);

    // 1. Male triggers mating
    ai.UpdateAI(mid, em, grid, 0.1f, rng);

    female = em.GetOrganism(fid);
    CHECK_TRUE(female->gestating);
    CHECK_EQ(female->father_id, mid);
    CHECK_EQ(female->father_scent, 100);
    CHECK_EQ(female->father_sight_range, 6);
    CHECK_EQ(female->father_smell_range, 10);

    // 2. Female finishes gestation and births child
    female->gestation_timer = 0.1f;
    ai.UpdateAI(fid, em, grid, 0.5f, rng);

    female = em.GetOrganism(fid);
    CHECK_FALSE(female->gestating);
    CHECK_EQ(em.ActiveOrganismCount(), 3u);

    // 3. Inspect child traits: sight avg (6+2)/2 = 4 (with +-1 jitter: [3, 5])
    // smell avg (10+4)/2 = 7 (with +-1 jitter: [6, 8])
    const auto& orgs = em.GetOrganisms();
    const Organism& child = orgs.back();
    CHECK_TRUE(child.sight_range >= 3 && child.sight_range <= 5);
    CHECK_TRUE(child.smell_range >= 6 && child.smell_range <= 8);
    // Scent should be near father (100) or mother (104) with minor mutation
    CHECK_TRUE(child.scent >= 90 && child.scent <= 115);
}

TEST_CASE(TC_PERF_01_BFS_Direct_Grid_Matching) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({30, 30}, Gender::MALE, 1, 1, 40, 10);
    grid.Occupy(30, 30, 1, 1, EntityType::ORGANISM, oid);

    // Place a food 4 cells away at (34, 30)
    uint32_t fid = em.CreateResource({34, 30}, EntityType::FOOD);
    grid.Occupy(34, 30, 1, 1, EntityType::FOOD, fid);

    Organism* org = em.GetOrganism(oid);
    org->hunger = 1.0f; // Critical need
    org->thirst = 10.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(oid, em, grid, 1.0f, rng);

    org = em.GetOrganism(oid);
    // Should step right towards food (x = 31)
    CHECK_EQ(org->pos.x, 31);
    CHECK_EQ(org->pos.y, 30);
}


