#pragma once

#include "sim/Common.hpp"
#include <algorithm>

namespace sim {

struct Resource {
    uint32_t id{0};
    Vector2i pos{0, 0};
    EntityType type{EntityType::FOOD};
    int calories{FOOD_CALORIES};
    uint16_t scent{SCENT_FOOD};
    bool alive{true};
};

struct Organism {
    uint32_t id{0};
    Vector2i pos{0, 0};
    int w{1};
    int h{1};
    Gender gender{Gender::MALE};
    float life_force{100.0f};
    float hunger{10.0f};
    float thirst{10.0f};
    int calories{40};
    int lifetime_calories{0};
    int age{1};
    int damage{0};
    uint16_t scent{1};
    int sight_range{4};
    int smell_range{6};
    bool gestating{false};
    float gestation_timer{0.0f};
    float age_timer{0.0f};
    float step_cooldown{0.0f};
    FSMState current_state{FSMState::WANDER};
    bool alive{true};

    void UpdateDamage() noexcept {
        int cells = w * h;
        damage = std::min(5, std::max(0, (cells - 1) / 3));
    }
};

} // namespace sim

