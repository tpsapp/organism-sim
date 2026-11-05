#include "sim/EntityManager.hpp"
#include <algorithm>

namespace sim {

EntityManager::EntityManager() = default;

uint32_t EntityManager::CreateResource(Vector2i pos, EntityType type) {
    uint32_t id = next_id_++;
    Resource res;
    res.id = id;
    res.pos = pos;
    res.type = type;
    res.alive = true;
    if (type == EntityType::FOOD) {
        res.calories = FOOD_CALORIES;
        res.scent = SCENT_FOOD;
    } else {
        res.calories = 0;
        res.scent = SCENT_WATER;
    }
    resources_.push_back(res);
    return id;
}

uint32_t EntityManager::CreateOrganism(Vector2i pos, Gender gender, int w, int h, int calories, uint16_t scent) {
    uint32_t id = next_id_++;
    Organism org;
    org.id = id;
    org.pos = pos;
    org.w = w;
    org.h = h;
    org.gender = gender;
    org.life_force = 100.0f;
    org.hunger = 10.0f;
    org.thirst = 10.0f;
    org.calories = calories;
    org.lifetime_calories = 0;
    org.age = 1;
    org.UpdateDamage();
    org.scent = (scent != 0) ? scent : static_cast<uint16_t>(id & 0x7FFF);
    if (org.scent == SCENT_FOOD || org.scent == SCENT_WATER) {
        org.scent = 1;
    }
    org.sight_range = 4;
    org.smell_range = 6;
    org.gestating = false;
    org.gestation_timer = 0.0f;
    org.age_timer = 0.0f;
    org.step_cooldown = 0.0f;
    org.current_state = FSMState::WANDER;
    org.alive = true;

    organisms_.push_back(org);
    return id;
}

Organism* EntityManager::GetOrganism(uint32_t id) noexcept {
    for (auto& org : organisms_) {
        if (org.id == id) return &org;
    }
    return nullptr;
}

const Organism* EntityManager::GetOrganism(uint32_t id) const noexcept {
    for (const auto& org : organisms_) {
        if (org.id == id) return &org;
    }
    return nullptr;
}

Resource* EntityManager::GetResource(uint32_t id) noexcept {
    for (auto& res : resources_) {
        if (res.id == id) return &res;
    }
    return nullptr;
}

const Resource* EntityManager::GetResource(uint32_t id) const noexcept {
    for (const auto& res : resources_) {
        if (res.id == id) return &res;
    }
    return nullptr;
}

void EntityManager::RemoveDeadEntities(GridMatrix& grid) {
    for (const auto& org : organisms_) {
        if (!org.alive) {
            grid.Vacate(org.pos.x, org.pos.y, org.w, org.h);
        }
    }
    for (const auto& res : resources_) {
        if (!res.alive) {
            grid.Vacate(res.pos.x, res.pos.y, 1, 1);
        }
    }

    std::erase_if(organisms_, [](const Organism& org) { return !org.alive; });
    std::erase_if(resources_, [](const Resource& res) { return !res.alive; });
}

size_t EntityManager::ActiveOrganismCount() const noexcept {
    size_t count = 0;
    for (const auto& org : organisms_) {
        if (org.alive) ++count;
    }
    return count;
}

size_t EntityManager::ActiveResourceCount() const noexcept {
    size_t count = 0;
    for (const auto& res : resources_) {
        if (res.alive) ++count;
    }
    return count;
}

void EntityManager::Clear() {
    organisms_.clear();
    resources_.clear();
    next_id_ = 1;
}

} // namespace sim

