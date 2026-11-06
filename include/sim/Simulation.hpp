#pragma once

#include "sim/Common.hpp"
#include "sim/GridMatrix.hpp"
#include "sim/Entity.hpp"
#include "sim/EntityManager.hpp"
#include "sim/AIEngine.hpp"
#include <random>

namespace sim {

class Simulation {
public:
    explicit Simulation(uint32_t seed = 42);

    void Reset();
    void Step(float dt);

    bool ManualPlace(int cell_x, int cell_y, EntityType type, Gender gender = Gender::MALE);

    [[nodiscard]] int RemainingMalePlacements() const noexcept { return remaining_males_; }
    [[nodiscard]] int RemainingFemalePlacements() const noexcept { return remaining_females_; }
    [[nodiscard]] GameStatus GetGameStatus() const noexcept { return status_; }
    [[nodiscard]] float OccupancyPercentage() const noexcept;

    [[nodiscard]] const GridMatrix& GetGrid() const noexcept { return grid_; }
    [[nodiscard]] GridMatrix& GetGrid() noexcept { return grid_; }

    [[nodiscard]] const EntityManager& GetEntities() const noexcept { return entity_mgr_; }
    [[nodiscard]] EntityManager& GetEntities() noexcept { return entity_mgr_; }

    [[nodiscard]] size_t MaleCount() const noexcept;
    [[nodiscard]] size_t FemaleCount() const noexcept;
    [[nodiscard]] size_t GestatingCount() const noexcept;
    [[nodiscard]] float AverageAge() const noexcept;

    [[nodiscard]] const Organism* GetOrganismAt(int cell_x, int cell_y) const noexcept;
    [[nodiscard]] uint64_t GetStepCount() const noexcept { return step_count_; }
    [[nodiscard]] const std::vector<PopulationSample>& GetPopulationHistory() const noexcept { return population_history_; }

    [[nodiscard]] bool SaveState(const std::string& filepath) const;
    bool LoadState(const std::string& filepath);

    void SetGameStatus(GameStatus s) noexcept { status_ = s; }

private:
    void SeedInitialWorld();
    void SpawnRandomResource();
    void UpdateMetabolismAndGrowth(float dt);

    GridMatrix grid_;
    EntityManager entity_mgr_;
    AIEngine ai_engine_;

    std::mt19937 rng_;
    uint64_t step_count_{0};
    float step_accumulator_{0.0f};
    int remaining_males_{MAX_MANUAL_MALES};
    int remaining_females_{MAX_MANUAL_FEMALES};
    float resource_spawn_timer_{60.0f};
    GameStatus status_{GameStatus::ONGOING};
    std::vector<PopulationSample> population_history_;
};

} // namespace sim

