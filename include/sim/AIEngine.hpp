#pragma once

#include "sim/Common.hpp"
#include "sim/Entity.hpp"
#include "sim/GridMatrix.hpp"
#include "sim/EntityManager.hpp"
#include <random>

namespace sim {

class AIEngine {
public:
    AIEngine() = default;

    static int ChebyshevDistance(const Vector2i& pos_a, int w_a, int h_a,
                                 const Vector2i& pos_b, int w_b, int h_b) noexcept;

    static bool AreAdjacent(const Vector2i& pos_a, int w_a, int h_a,
                            const Vector2i& pos_b, int w_b, int h_b) noexcept;

    static bool AreScentsCompatible(uint16_t scent_a, uint16_t scent_b) noexcept;

    void UpdateAI(uint32_t org_id, EntityManager& entity_mgr, GridMatrix& grid, float dt, std::mt19937& rng);
    void UpdateAI(Organism& org, EntityManager& entity_mgr, GridMatrix& grid, float dt, std::mt19937& rng);

    static Organism CreateOffspring(const Organism& mother, const Organism& father, Vector2i birth_pos, uint32_t id, std::mt19937& rng);
};

} // namespace sim

