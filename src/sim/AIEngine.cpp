#include "sim/AIEngine.hpp"
#include <cstdlib>
#include <limits>
#include <vector>

namespace sim {

int AIEngine::ChebyshevDistance(const Vector2i& pos_a, int w_a, int h_a,
                                const Vector2i& pos_b, int w_b, int h_b) noexcept {
    int dx = std::max(0, std::max(pos_a.x - (pos_b.x + w_b - 1), pos_b.x - (pos_a.x + w_a - 1)));
    int dy = std::max(0, std::max(pos_a.y - (pos_b.y + h_b - 1), pos_b.y - (pos_a.y + h_a - 1)));
    return std::max(dx, dy);
}

bool AIEngine::AreAdjacent(const Vector2i& pos_a, int w_a, int h_a,
                           const Vector2i& pos_b, int w_b, int h_b) noexcept {
    return ChebyshevDistance(pos_a, w_a, h_a, pos_b, w_b, h_b) <= 1;
}

bool AIEngine::AreScentsCompatible(uint16_t scent_a, uint16_t scent_b) noexcept {
    int diff = std::abs(static_cast<int>(scent_a) - static_cast<int>(scent_b));
    return (diff % 10) < 7;
}

Organism AIEngine::CreateOffspring(const Organism& mother, const Organism& father,
                                   Vector2i birth_pos, uint32_t id, std::mt19937& rng) {
    Organism child;
    child.id = id;
    child.pos = birth_pos;
    child.w = 1;
    child.h = 1;

    std::uniform_int_distribution<int> gender_dist(0, 1);
    child.gender = (gender_dist(rng) == 0) ? Gender::MALE : Gender::FEMALE;

    std::uniform_int_distribution<int> jitter_dist(-1, 1);
    int avg_sight = (mother.sight_range + father.sight_range) / 2;
    child.sight_range = std::clamp(avg_sight + jitter_dist(rng), 2, 6);

    int avg_smell = (mother.smell_range + father.smell_range) / 2;
    child.smell_range = std::clamp(avg_smell + jitter_dist(rng), 4, 10);

    std::uniform_int_distribution<int> cal_dist(20, 60);
    child.calories = cal_dist(rng);
    child.lifetime_calories = 0;

    std::uniform_int_distribution<int> scent_dist(1, 0xFFFD);
    child.scent = static_cast<uint16_t>(scent_dist(rng));
    if (child.scent == SCENT_FOOD || child.scent == SCENT_WATER) {
        child.scent = 1;
    }

    child.age = 1;
    child.life_force = 100.0f;
    child.hunger = 10.0f;
    child.thirst = 10.0f;
    child.gestating = false;
    child.gestation_timer = 0.0f;
    child.age_timer = 0.0f;
    child.step_cooldown = 0.0f;
    child.current_state = FSMState::WANDER;
    child.alive = true;
    child.UpdateDamage();

    return child;
}

void AIEngine::UpdateAI(Organism& org, EntityManager& entity_mgr, GridMatrix& grid,
                        float dt, std::mt19937& rng) {
    UpdateAI(org.id, entity_mgr, grid, dt, rng);
}

namespace {

struct PathResult {
    bool found{false};
    Vector2i first_step{0, 0};
    Vector2i target_pos{0, 0};
    int target_w{1};
    int target_h{1};
    uint32_t target_id{0};
};

PathResult FindReachableResource(const Organism& org, const EntityManager& em, const GridMatrix& grid, EntityType desired_type) {
    PathResult result;

    // 1. Check if already adjacent to any matching resource
    int best_adj_dist = std::numeric_limits<int>::max();
    for (const auto& res : em.GetResources()) {
        if (!res.alive || res.type != desired_type) continue;
        int dist = AIEngine::ChebyshevDistance(org.pos, org.w, org.h, res.pos, 1, 1);
        if (dist <= 1 && dist < best_adj_dist) {
            best_adj_dist = dist;
            result.found = true;
            result.first_step = {0, 0};
            result.target_pos = res.pos;
            result.target_w = 1;
            result.target_h = 1;
            result.target_id = res.id;
        }
    }
    if (result.found) return result;

    // 2. BFS for closest reachable matching resource within smell_range
    int min_x = std::max(0, org.pos.x - org.smell_range);
    int max_x = std::min(GRID_WIDTH - org.w, org.pos.x + org.smell_range);
    int min_y = std::max(0, org.pos.y - org.smell_range);
    int max_y = std::min(GRID_HEIGHT - org.h, org.pos.y + org.smell_range);

    constexpr int MAX_SPAN = 30;
    bool visited[MAX_SPAN][MAX_SPAN] = {};

    auto to_local_x = [&](int gx) { return gx - min_x; };
    auto to_local_y = [&](int gy) { return gy - min_y; };

    visited[to_local_x(org.pos.x)][to_local_y(org.pos.y)] = true;

    struct Node {
        Vector2i pos;
        Vector2i first_step;
    };

    static const Vector2i kDirs[8] = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0},
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    };

    std::vector<Node> queue;
    queue.reserve(500);

    for (const auto& dir : kDirs) {
        Vector2i next_pos = org.pos + dir;
        if (next_pos.x >= min_x && next_pos.x <= max_x &&
            next_pos.y >= min_y && next_pos.y <= max_y) {
            if (grid.CanPlace(next_pos.x, next_pos.y, org.w, org.h, org.id)) {
                visited[to_local_x(next_pos.x)][to_local_y(next_pos.y)] = true;
                queue.push_back({next_pos, dir});
            }
        }
    }

    size_t head = 0;
    while (head < queue.size()) {
        Node curr = queue[head++];

        // Check if curr is adjacent to any matching resource within smell_range
        for (const auto& res : em.GetResources()) {
            if (!res.alive || res.type != desired_type) continue;
            if (AIEngine::ChebyshevDistance(org.pos, org.w, org.h, res.pos, 1, 1) > org.smell_range) continue;
            if (AIEngine::AreAdjacent(curr.pos, org.w, org.h, res.pos, 1, 1)) {
                result.found = true;
                result.first_step = curr.first_step;
                result.target_pos = res.pos;
                result.target_w = 1;
                result.target_h = 1;
                result.target_id = res.id;
                return result;
            }
        }

        // Expand neighbors
        for (const auto& dir : kDirs) {
            Vector2i next_pos = curr.pos + dir;
            if (next_pos.x >= min_x && next_pos.x <= max_x &&
                next_pos.y >= min_y && next_pos.y <= max_y) {
                int lx = to_local_x(next_pos.x);
                int ly = to_local_y(next_pos.y);
                if (!visited[lx][ly]) {
                    visited[lx][ly] = true;
                    if (grid.CanPlace(next_pos.x, next_pos.y, org.w, org.h, org.id)) {
                        queue.push_back({next_pos, curr.first_step});
                    }
                }
            }
        }
    }

    return result;
}

PathResult FindReachableMate(const Organism& org, const EntityManager& em, const GridMatrix& grid) {
    PathResult result;
    Gender opp_gender = (org.gender == Gender::MALE) ? Gender::FEMALE : Gender::MALE;

    // 1. Check adjacent mate first
    for (const auto& other : em.GetOrganisms()) {
        if (!other.alive || other.id == org.id || other.gender != opp_gender) continue;
        if (other.age < REPRODUCTION_MIN_AGE || other.gestating) continue;
        if (other.hunger <= REPRODUCTION_MIN_HUNGER || other.thirst <= REPRODUCTION_MIN_THIRST) continue;

        if (AIEngine::AreAdjacent(org.pos, org.w, org.h, other.pos, other.w, other.h)) {
            result.found = true;
            result.first_step = {0, 0};
            result.target_pos = other.pos;
            result.target_w = other.w;
            result.target_h = other.h;
            result.target_id = other.id;
            return result;
        }
    }

    // 2. BFS for reachable mate
    int min_x = std::max(0, org.pos.x - org.smell_range);
    int max_x = std::min(GRID_WIDTH - org.w, org.pos.x + org.smell_range);
    int min_y = std::max(0, org.pos.y - org.smell_range);
    int max_y = std::min(GRID_HEIGHT - org.h, org.pos.y + org.smell_range);

    constexpr int MAX_SPAN = 30;
    bool visited[MAX_SPAN][MAX_SPAN] = {};

    auto to_local_x = [&](int gx) { return gx - min_x; };
    auto to_local_y = [&](int gy) { return gy - min_y; };

    visited[to_local_x(org.pos.x)][to_local_y(org.pos.y)] = true;

    struct Node {
        Vector2i pos;
        Vector2i first_step;
    };

    static const Vector2i kDirs[8] = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0},
        {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
    };

    std::vector<Node> queue;
    queue.reserve(500);

    for (const auto& dir : kDirs) {
        Vector2i next_pos = org.pos + dir;
        if (next_pos.x >= min_x && next_pos.x <= max_x &&
            next_pos.y >= min_y && next_pos.y <= max_y) {
            if (grid.CanPlace(next_pos.x, next_pos.y, org.w, org.h, org.id)) {
                visited[to_local_x(next_pos.x)][to_local_y(next_pos.y)] = true;
                queue.push_back({next_pos, dir});
            }
        }
    }

    size_t head = 0;
    while (head < queue.size()) {
        Node curr = queue[head++];

        for (const auto& other : em.GetOrganisms()) {
            if (!other.alive || other.id == org.id || other.gender != opp_gender) continue;
            if (other.age < REPRODUCTION_MIN_AGE || other.gestating) continue;
            if (other.hunger <= REPRODUCTION_MIN_HUNGER || other.thirst <= REPRODUCTION_MIN_THIRST) continue;
            if (AIEngine::ChebyshevDistance(org.pos, org.w, org.h, other.pos, other.w, other.h) > org.smell_range) continue;

            if (AIEngine::AreAdjacent(curr.pos, org.w, org.h, other.pos, other.w, other.h)) {
                result.found = true;
                result.first_step = curr.first_step;
                result.target_pos = other.pos;
                result.target_w = other.w;
                result.target_h = other.h;
                result.target_id = other.id;
                return result;
            }
        }

        for (const auto& dir : kDirs) {
            Vector2i next_pos = curr.pos + dir;
            if (next_pos.x >= min_x && next_pos.x <= max_x &&
                next_pos.y >= min_y && next_pos.y <= max_y) {
                int lx = to_local_x(next_pos.x);
                int ly = to_local_y(next_pos.y);
                if (!visited[lx][ly]) {
                    visited[lx][ly] = true;
                    if (grid.CanPlace(next_pos.x, next_pos.y, org.w, org.h, org.id)) {
                        queue.push_back({next_pos, curr.first_step});
                    }
                }
            }
        }
    }

    return result;
}

} // anonymous namespace

void AIEngine::UpdateAI(uint32_t org_id, EntityManager& entity_mgr, GridMatrix& grid,
                        float dt, std::mt19937& rng) {
    Organism* p_org = entity_mgr.GetOrganism(org_id);
    if (!p_org || !p_org->alive) return;

    // Handle gestation timer if female
    if (p_org->gestating) {
        p_org->gestation_timer -= dt;
        if (p_org->gestation_timer <= 0.0f) {
            // Find adjacent empty 1x1 cell
            bool birthed = false;
            static const Vector2i kOffsets[8] = {
                {0, -1}, {0, 1}, {-1, 0}, {1, 0},
                {-1, -1}, {1, -1}, {-1, 1}, {1, 1}
            };
            for (const auto& off : kOffsets) {
                Vector2i birth_pos = p_org->pos + off;
                if (grid.CanPlace(birth_pos.x, birth_pos.y, 1, 1)) {
                    Organism mother_copy = *p_org;
                    uint32_t child_id = entity_mgr.CreateOrganism(birth_pos, Gender::MALE);
                    Organism* child = entity_mgr.GetOrganism(child_id);
                    if (child) {
                        *child = CreateOffspring(mother_copy, mother_copy, birth_pos, child_id, rng);
                        grid.Occupy(birth_pos.x, birth_pos.y, 1, 1, EntityType::ORGANISM, child_id);
                    }
                    p_org = entity_mgr.GetOrganism(org_id);
                    if (p_org) {
                        p_org->gestating = false;
                        p_org->gestation_timer = 0.0f;
                    }
                    birthed = true;
                    break;
                }
            }
            if (!birthed) {
                p_org = entity_mgr.GetOrganism(org_id);
                if (p_org) p_org->gestation_timer = 0.0f;
            }
        }
    }

    p_org = entity_mgr.GetOrganism(org_id);
    if (!p_org || !p_org->alive) return;
    auto& org = *p_org;

    // Update step cooldown
    if (org.step_cooldown > 0.0f) {
        org.step_cooldown -= dt;
        if (org.step_cooldown > 0.0f) {
            return;
        }
    }

    // Evaluate FSM State
    bool is_critical = (std::min(org.hunger, org.thirst) <= CRITICAL_NEED_THRESHOLD);
    bool can_mate = (org.hunger > REPRODUCTION_MIN_HUNGER &&
                     org.thirst > REPRODUCTION_MIN_THIRST &&
                     org.age >= REPRODUCTION_MIN_AGE &&
                     !org.gestating);

    Vector2i target_pos{org.pos};
    int target_w = 1;
    int target_h = 1;
    bool has_target = false;
    uint32_t target_resource_id = 0;
    uint32_t target_mate_id = 0;
    Vector2i chosen_step{0, 0};

    if (is_critical) {
        org.current_state = FSMState::CRITICAL_NEED;
        EntityType desired_type = (org.thirst <= org.hunger) ? EntityType::WATER : EntityType::FOOD;
        EntityType alt_type = (desired_type == EntityType::WATER) ? EntityType::FOOD : EntityType::WATER;

        PathResult pr = FindReachableResource(org, entity_mgr, grid, desired_type);
        if (!pr.found) {
            pr = FindReachableResource(org, entity_mgr, grid, alt_type);
        }

        if (pr.found) {
            has_target = true;
            chosen_step = pr.first_step;
            target_pos = pr.target_pos;
            target_w = pr.target_w;
            target_h = pr.target_h;
            target_resource_id = pr.target_id;
        }
    } else if (can_mate) {
        org.current_state = FSMState::SEEK_MATE;
        PathResult pr = FindReachableMate(org, entity_mgr, grid);
        if (pr.found) {
            has_target = true;
            chosen_step = pr.first_step;
            target_pos = pr.target_pos;
            target_w = pr.target_w;
            target_h = pr.target_h;
            target_mate_id = pr.target_id;
        } else {
            org.current_state = FSMState::WANDER;
        }
    } else {
        org.current_state = FSMState::WANDER;
    }

    // Check interaction if already adjacent to target
    if (has_target && AreAdjacent(org.pos, org.w, org.h, target_pos, target_w, target_h)) {
        if (target_resource_id != 0) {
            Resource* res = entity_mgr.GetResource(target_resource_id);
            if (res && res->alive) {
                if (res->type == EntityType::FOOD) {
                    org.hunger = 10.0f;
                    org.calories += res->calories;
                    org.lifetime_calories += res->calories;
                    std::uniform_real_distribution<float> heal_dist(MIN_FOOD_HEAL, MAX_FOOD_HEAL);
                    org.life_force = std::min(100.0f, org.life_force + heal_dist(rng));
                } else if (res->type == EntityType::WATER) {
                    org.thirst = 10.0f;
                }
                res->alive = false;
                grid.Vacate(res->pos.x, res->pos.y, 1, 1);
                return;
            }
        } else if (target_mate_id != 0) {
            Organism* mate = entity_mgr.GetOrganism(target_mate_id);
            if (mate && mate->alive && AreScentsCompatible(org.scent, mate->scent)) {
                Organism* female = (org.gender == Gender::FEMALE) ? &org : mate;
                if (!female->gestating) {
                    female->gestating = true;
                    female->gestation_timer = GESTATION_TIME;
                }
                return;
            }
        }
    }

    // Execute planned path step
    if (has_target && (chosen_step.x != 0 || chosen_step.y != 0)) {
        Vector2i new_pos = org.pos + chosen_step;
        if (grid.CanPlace(new_pos.x, new_pos.y, org.w, org.h, org.id)) {
            grid.Vacate(org.pos.x, org.pos.y, org.w, org.h);
            org.pos = new_pos;
            grid.Occupy(org.pos.x, org.pos.y, org.w, org.h, EntityType::ORGANISM, org.id);
            org.calories = std::max(0, org.calories - CALORIE_MOVE_COST);
            org.step_cooldown = BASE_STEP_INTERVAL * (org.calories == 0 ? 2.0f : 1.0f);
            return;
        }
    }

    // Wandering step
    std::uniform_int_distribution<int> dir_dist(-1, 1);
    Vector2i desired_dir{dir_dist(rng), dir_dist(rng)};
    if (desired_dir.x == 0 && desired_dir.y == 0) {
        return;
    }

    static const Vector2i kAllDirs[8] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };

    std::vector<Vector2i> candidates;
    candidates.push_back(desired_dir);
    for (const auto& dir : kAllDirs) {
        if (dir != desired_dir) candidates.push_back(dir);
    }

    for (const auto& d : candidates) {
        Vector2i new_pos = org.pos + d;
        if (grid.CanPlace(new_pos.x, new_pos.y, org.w, org.h, org.id)) {
            grid.Vacate(org.pos.x, org.pos.y, org.w, org.h);
            org.pos = new_pos;
            grid.Occupy(org.pos.x, org.pos.y, org.w, org.h, EntityType::ORGANISM, org.id);
            org.calories = std::max(0, org.calories - CALORIE_MOVE_COST);
            org.step_cooldown = BASE_STEP_INTERVAL * (org.calories == 0 ? 2.0f : 1.0f);
            break;
        }
    }
}

} // namespace sim

