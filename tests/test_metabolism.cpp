#include "TestHarness.hpp"
#include "sim/Simulation.hpp"

using namespace sim;

TEST_CASE(TC_MET_01_Thirst_Depletion) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.thirst = 10.0f;
    org.hunger = 10.0f;
    org.calories = 50;

    sim.Step(1.0f); // 1.0 second elapsed
    CHECK_NEAR(org.thirst, 9.90f, 0.01);
}

TEST_CASE(TC_MET_02_Hunger_Depletion) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.thirst = 10.0f;
    org.hunger = 10.0f;
    org.calories = 50;

    sim.Step(1.0f);
    CHECK_NEAR(org.hunger, 9.95f, 0.01);
}

TEST_CASE(TC_MET_03_Movement_Calorie_Drain) {
    GridMatrix grid;
    EntityManager em;
    uint32_t id = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 50);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, id);

    AIEngine ai;
    std::mt19937 rng(42);
    Organism* org = em.GetOrganism(id);
    org->step_cooldown = 0.0f;

    // Place water at 12, 10 so it steps towards it
    uint32_t wid = em.CreateResource({12, 10}, EntityType::WATER);
    grid.Occupy(12, 10, 1, 1, EntityType::WATER, wid);
    org->thirst = 2.0f; // Critical need

    ai.UpdateAI(*org, em, grid, 1.0f, rng);
    CHECK_EQ(org->calories, 49);
}

TEST_CASE(TC_MET_04_Exhaustion_Penalties) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.calories = 0; // Exhausted
    org.hunger = 10.0f;
    org.thirst = 10.0f;

    sim.Step(1.0f);
    // Hunger rate should be 0.05 * 1.5 = 0.075
    CHECK_NEAR(org.hunger, 10.0f - 0.075f, 0.01);
}

TEST_CASE(TC_MET_05_Starvation_Damage) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.hunger = 0.0f;
    org.thirst = 10.0f;
    org.life_force = 100.0f;

    sim.Step(1.0f);
    CHECK_NEAR(org.life_force, 98.0f, 0.01);
}

TEST_CASE(TC_MET_06_Dehydration_Damage) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.thirst = 0.0f;
    org.hunger = 10.0f;
    org.life_force = 100.0f;

    sim.Step(1.0f);
    CHECK_NEAR(org.life_force, 98.0f, 0.01);
}

TEST_CASE(TC_MET_07_Simultaneous_Drain) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.hunger = 0.0f;
    org.thirst = 0.0f;
    org.life_force = 100.0f;

    sim.Step(1.0f);
    CHECK_NEAR(org.life_force, 98.0f, 0.01);
}

TEST_CASE(TC_MET_08_Mortality_And_Despawn) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.hunger = 0.0f;
    org.life_force = 1.0f;

    Vector2i dead_pos = org.pos;
    sim.Step(1.0f); // Takes 2 damage, life_force <= 0, despawns
    CHECK_EQ(static_cast<int>(sim.GetGrid().At(dead_pos.x, dead_pos.y).type), static_cast<int>(EntityType::EMPTY));
}

TEST_CASE(TC_MET_09_Aging_Transition) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.age = 1;
    org.age_timer = 359.5f;

    sim.Step(1.0f);
    CHECK_EQ(org.age, 2);
}

TEST_CASE(TC_MET_10_Food_Ingestion) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 20);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, oid);

    uint32_t fid = em.CreateResource({11, 10}, EntityType::FOOD);
    grid.Occupy(11, 10, 1, 1, EntityType::FOOD, fid);

    Organism* org = em.GetOrganism(oid);
    org->hunger = 2.0f;
    org->step_cooldown = 0.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*org, em, grid, 1.0f, rng);

    CHECK_NEAR(org->hunger, 10.0f, 0.01);
    CHECK_EQ(org->calories, 60); // 20 + 40
    CHECK_FALSE(em.GetResource(fid)->alive);
}

TEST_CASE(TC_MET_11_Water_Ingestion) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({10, 10}, Gender::FEMALE, 1, 1, 30);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, oid);

    uint32_t wid = em.CreateResource({10, 11}, EntityType::WATER);
    grid.Occupy(10, 11, 1, 1, EntityType::WATER, wid);

    Organism* org = em.GetOrganism(oid);
    org->thirst = 1.0f;
    org->step_cooldown = 0.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*org, em, grid, 1.0f, rng);

    CHECK_NEAR(org->thirst, 10.0f, 0.01);
    CHECK_FALSE(em.GetResource(wid)->alive);
}

TEST_CASE(TC_MET_12_Passive_Healing_Active) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.hunger = 8.0f;
    org.thirst = 8.0f;
    org.life_force = 50.0f;

    sim.Step(1.0f);
    // Passive regen rate is 1.0f/s; hunger and thirst stay > 5.0f after 1.0s drain
    CHECK_NEAR(org.life_force, 51.0f, 0.01);
}

TEST_CASE(TC_MET_13_Passive_Healing_Threshold) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.hunger = 5.0f;
    org.thirst = 8.0f;
    org.life_force = 50.0f;

    sim.Step(1.0f);
    // Hunger is <= 5.0f, so passive healing must not trigger
    CHECK_NEAR(org.life_force, 50.0f, 0.01);
}

TEST_CASE(TC_MET_14_Passive_Healing_Max_Clamp) {
    Simulation sim(123);
    auto& organisms = sim.GetEntities().GetOrganisms();
    Organism& org = organisms.front();
    org.hunger = 8.0f;
    org.thirst = 8.0f;
    org.life_force = 99.5f;

    sim.Step(1.0f);
    // 99.5f + 1.0f = 100.5f -> clamped strictly to 100.0f
    CHECK_NEAR(org.life_force, 100.0f, 0.01);
}

TEST_CASE(TC_MET_15_Nutritional_Food_Healing) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 20);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, oid);

    uint32_t fid = em.CreateResource({11, 10}, EntityType::FOOD);
    grid.Occupy(11, 10, 1, 1, EntityType::FOOD, fid);

    Organism* org = em.GetOrganism(oid);
    org->hunger = 2.0f;
    org->life_force = 50.0f;
    org->step_cooldown = 0.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*org, em, grid, 1.0f, rng);

    CHECK_NEAR(org->hunger, 10.0f, 0.01);
    CHECK_TRUE(org->life_force >= 55.0f && org->life_force <= 65.0f);
    CHECK_FALSE(em.GetResource(fid)->alive);
}

TEST_CASE(TC_MET_16_Food_Healing_Max_Clamp) {
    GridMatrix grid;
    EntityManager em;
    uint32_t oid = em.CreateOrganism({10, 10}, Gender::MALE, 1, 1, 20);
    grid.Occupy(10, 10, 1, 1, EntityType::ORGANISM, oid);

    uint32_t fid = em.CreateResource({11, 10}, EntityType::FOOD);
    grid.Occupy(11, 10, 1, 1, EntityType::FOOD, fid);

    Organism* org = em.GetOrganism(oid);
    org->hunger = 2.0f;
    org->life_force = 98.0f;
    org->step_cooldown = 0.0f;

    AIEngine ai;
    std::mt19937 rng(42);
    ai.UpdateAI(*org, em, grid, 1.0f, rng);

    CHECK_NEAR(org->hunger, 10.0f, 0.01);
    CHECK_NEAR(org->life_force, 100.0f, 0.001);
    CHECK_FALSE(em.GetResource(fid)->alive);
}

