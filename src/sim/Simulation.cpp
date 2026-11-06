#include "sim/Simulation.hpp"
#include "JsonHelper.hpp"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <sstream>

namespace sim {

Simulation::Simulation(uint32_t seed)
    : rng_(seed) {
    Reset();
}

void Simulation::Reset() {
    grid_.Clear();
    entity_mgr_.Clear();
    step_count_ = 0;
    step_accumulator_ = 0.0f;
    remaining_males_ = MAX_MANUAL_MALES;
    remaining_females_ = MAX_MANUAL_FEMALES;
    status_ = GameStatus::ONGOING;
    population_history_.clear();

    std::uniform_real_distribution<float> spawn_dist(MIN_RESOURCE_SPAWN_INTERVAL, MAX_RESOURCE_SPAWN_INTERVAL);
    resource_spawn_timer_ = spawn_dist(rng_);

    SeedInitialWorld();

    // Initial baseline sample
    population_history_.push_back({
        static_cast<int>(MaleCount()),
        static_cast<int>(FemaleCount()),
        INITIAL_FOOD_COUNT
    });
}

void Simulation::SeedInitialWorld() {
    std::uniform_int_distribution<int> coord_dist(0, GRID_WIDTH - 1);

    auto find_empty_cell = [&]() -> Vector2i {
        for (int attempts = 0; attempts < 1000; ++attempts) {
            int x = coord_dist(rng_);
            int y = coord_dist(rng_);
            if (grid_.CanPlace(x, y, 1, 1)) {
                return {x, y};
            }
        }
        for (int x = 0; x < GRID_WIDTH; ++x) {
            for (int y = 0; y < GRID_HEIGHT; ++y) {
                if (grid_.CanPlace(x, y, 1, 1)) {
                    return {x, y};
                }
            }
        }
        return {-1, -1};
    };

    // Place Initial Food
    for (int i = 0; i < INITIAL_FOOD_COUNT; ++i) {
        Vector2i pos = find_empty_cell();
        if (pos.x >= 0) {
            uint32_t id = entity_mgr_.CreateResource(pos, EntityType::FOOD);
            grid_.Occupy(pos.x, pos.y, 1, 1, EntityType::FOOD, id);
        }
    }

    // Place Initial Water
    for (int i = 0; i < INITIAL_WATER_COUNT; ++i) {
        Vector2i pos = find_empty_cell();
        if (pos.x >= 0) {
            uint32_t id = entity_mgr_.CreateResource(pos, EntityType::WATER);
            grid_.Occupy(pos.x, pos.y, 1, 1, EntityType::WATER, id);
        }
    }

    // Place 1 Initial Male Organism
    Vector2i m_pos = find_empty_cell();
    if (m_pos.x >= 0) {
        uint32_t m_id = entity_mgr_.CreateOrganism(m_pos, Gender::MALE, 1, 1, 40, 10);
        grid_.Occupy(m_pos.x, m_pos.y, 1, 1, EntityType::ORGANISM, m_id);
    }

    // Place 1 Initial Female Organism
    Vector2i f_pos = find_empty_cell();
    if (f_pos.x >= 0) {
        uint32_t f_id = entity_mgr_.CreateOrganism(f_pos, Gender::FEMALE, 1, 1, 40, 14);
        grid_.Occupy(f_pos.x, f_pos.y, 1, 1, EntityType::ORGANISM, f_id);
    }
}

void Simulation::SpawnRandomResource() {
    std::uniform_int_distribution<int> coord_dist(0, GRID_WIDTH - 1);
    Vector2i pos{-1, -1};

    for (int attempts = 0; attempts < 500; ++attempts) {
        int x = coord_dist(rng_);
        int y = coord_dist(rng_);
        if (grid_.CanPlace(x, y, 1, 1)) {
            pos = {x, y};
            break;
        }
    }

    if (pos.x < 0) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            for (int y = 0; y < GRID_HEIGHT; ++y) {
                if (grid_.CanPlace(x, y, 1, 1)) {
                    pos = {x, y};
                    break;
                }
            }
            if (pos.x >= 0) break;
        }
    }

    if (pos.x >= 0) {
        std::uniform_int_distribution<int> type_dist(0, 1);
        EntityType type = (type_dist(rng_) == 0) ? EntityType::FOOD : EntityType::WATER;
        uint32_t id = entity_mgr_.CreateResource(pos, type);
        grid_.Occupy(pos.x, pos.y, 1, 1, type, id);
    }
}

void Simulation::UpdateMetabolismAndGrowth(float dt) {
    for (auto& org : entity_mgr_.GetOrganisms()) {
        if (!org.alive) continue;

        // Thirst drain: 0.10f / s
        org.thirst = std::max(0.0f, org.thirst - THIRST_DEPLETION_RATE * dt);

        // Hunger drain: 0.05f / s (or 1.5x if exhausted/0 calories)
        float hunger_rate = (org.calories == 0) ? (HUNGER_DEPLETION_RATE * 1.5f) : HUNGER_DEPLETION_RATE;
        org.hunger = std::max(0.0f, org.hunger - hunger_rate * dt);

        // Starvation/Dehydration damage
        if (org.thirst <= 0.0f || org.hunger <= 0.0f) {
            org.life_force = std::max(0.0f, org.life_force - STARVATION_DAMAGE_RATE * dt);
            if (org.life_force <= 0.0f) {
                org.alive = false;
                continue;
            }
        } else if (org.hunger > PASSIVE_HEAL_THRESHOLD && org.thirst > PASSIVE_HEAL_THRESHOLD && org.life_force < 100.0f) {
            // Passive Regeneration: heal up to 100 life force when hunger & thirst > 50%
            org.life_force = std::min(100.0f, org.life_force + PASSIVE_HEAL_RATE * dt);
        }

        // Aging
        org.age_timer += dt;
        if (org.age_timer >= AGING_INTERVAL) {
            org.age_timer -= AGING_INTERVAL;
            if (org.age < 100) {
                org.age += 1;
            }
        }

        // Growth Check (Milestones: 80, 200, 450 calories, age >= 2, max 4x4)
        if (org.age >= 2 && (org.w < 4 || org.h < 4)) {
            int area = org.w * org.h;
            int target_area = area;
            if (area == 1 && org.lifetime_calories >= 80) {
                target_area = 2;
            } else if (area == 2 && org.lifetime_calories >= 200) {
                target_area = 4;
            } else if (area == 4 && org.lifetime_calories >= 450) {
                target_area = 9;
            }

            if (target_area > area) {
                int next_w = org.w;
                int next_h = org.h;
                if (next_w <= next_h && next_w < 4) {
                    next_w += 1;
                } else if (next_h < 4) {
                    next_h += 1;
                }

                if (grid_.CanExpand(org.pos.x, org.pos.y, org.w, org.h, next_w, next_h, org.id)) {
                    grid_.Occupy(org.pos.x, org.pos.y, next_w, next_h, EntityType::ORGANISM, org.id);
                    org.w = next_w;
                    org.h = next_h;
                    org.UpdateDamage();
                }
            }
        }
    }
}

void Simulation::Step(float dt) {
    if (status_ != GameStatus::ONGOING) {
        return;
    }

    step_accumulator_ += dt;
    while (step_accumulator_ >= (BASE_STEP_INTERVAL - 1e-5f)) {
        step_accumulator_ = std::max(0.0f, step_accumulator_ - BASE_STEP_INTERVAL);
        ++step_count_;

        int food_count = 0;
        for (const auto& res : entity_mgr_.GetResources()) {
            if (res.alive && res.type == EntityType::FOOD) {
                ++food_count;
            }
        }
        population_history_.push_back({
            static_cast<int>(MaleCount()),
            static_cast<int>(FemaleCount()),
            food_count
        });
        if (population_history_.size() > 100) {
            population_history_.erase(population_history_.begin());
        }
    }

    // 1. Update metabolism, starvation damage, and growth
    UpdateMetabolismAndGrowth(dt);

    // 2. Update AI behaviors and navigation (snapshot active IDs to prevent iterator invalidation on birth)
    std::vector<uint32_t> active_ids;
    active_ids.reserve(entity_mgr_.GetOrganisms().size());
    for (const auto& org : entity_mgr_.GetOrganisms()) {
        if (org.alive) {
            active_ids.push_back(org.id);
        }
    }
    for (uint32_t id : active_ids) {
        ai_engine_.UpdateAI(id, entity_mgr_, grid_, dt, rng_);
    }

    // 3. Periodic resource spawning
    resource_spawn_timer_ -= dt;
    if (resource_spawn_timer_ <= 0.0f) {
        SpawnRandomResource();
        std::uniform_real_distribution<float> spawn_dist(MIN_RESOURCE_SPAWN_INTERVAL, MAX_RESOURCE_SPAWN_INTERVAL);
        resource_spawn_timer_ = spawn_dist(rng_);
    }

    // 4. Prune dead entities and clear grid cells
    entity_mgr_.RemoveDeadEntities(grid_);

    // 5. Check Win / Loss conditions
    if (entity_mgr_.ActiveOrganismCount() == 0) {
        status_ = GameStatus::DEFEAT;
    } else if (grid_.OccupiedCellCount() >= VICTORY_CELL_THRESHOLD) {
        status_ = GameStatus::VICTORY;
    }
}

bool Simulation::ManualPlace(int cell_x, int cell_y, EntityType type, Gender gender) {
    if (!grid_.InBounds(cell_x, cell_y) || !grid_.CanPlace(cell_x, cell_y, 1, 1)) {
        return false;
    }

    if (type == EntityType::ORGANISM) {
        if (gender == Gender::MALE) {
            if (remaining_males_ <= 0) return false;
            --remaining_males_;
        } else {
            if (remaining_females_ <= 0) return false;
            --remaining_females_;
        }
        uint32_t id = entity_mgr_.CreateOrganism({cell_x, cell_y}, gender, 1, 1, 40);
        grid_.Occupy(cell_x, cell_y, 1, 1, EntityType::ORGANISM, id);
        return true;
    } else if (type == EntityType::FOOD || type == EntityType::WATER) {
        uint32_t id = entity_mgr_.CreateResource({cell_x, cell_y}, type);
        grid_.Occupy(cell_x, cell_y, 1, 1, type, id);
        return true;
    }

    return false;
}

float Simulation::OccupancyPercentage() const noexcept {
    constexpr float total_cells = static_cast<float>(GRID_WIDTH * GRID_HEIGHT);
    return (static_cast<float>(grid_.OccupiedCellCount()) / total_cells) * 100.0f;
}

size_t Simulation::MaleCount() const noexcept {
    size_t count = 0;
    for (const auto& org : entity_mgr_.GetOrganisms()) {
        if (org.alive && org.gender == Gender::MALE) ++count;
    }
    return count;
}

size_t Simulation::FemaleCount() const noexcept {
    size_t count = 0;
    for (const auto& org : entity_mgr_.GetOrganisms()) {
        if (org.alive && org.gender == Gender::FEMALE) ++count;
    }
    return count;
}

size_t Simulation::GestatingCount() const noexcept {
    size_t count = 0;
    for (const auto& org : entity_mgr_.GetOrganisms()) {
        if (org.alive && org.gestating) ++count;
    }
    return count;
}

float Simulation::AverageAge() const noexcept {
    size_t count = 0;
    int total_age = 0;
    for (const auto& org : entity_mgr_.GetOrganisms()) {
        if (org.alive) {
            ++count;
            total_age += org.age;
        }
    }
    return (count == 0) ? 0.0f : (static_cast<float>(total_age) / static_cast<float>(count));
}

const Organism* Simulation::GetOrganismAt(int cell_x, int cell_y) const noexcept {
    if (!grid_.InBounds(cell_x, cell_y)) {
        return nullptr;
    }
    const Cell& c = grid_.At(cell_x, cell_y);
    if (c.type != EntityType::ORGANISM || c.entity_id == 0) {
        return nullptr;
    }
    return entity_mgr_.GetOrganism(c.entity_id);
}

bool Simulation::SaveState(const std::string& filepath) const {
    std::ofstream out(filepath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"step_count\": " << step_count_ << ",\n";
    out << "  \"step_accumulator\": " << step_accumulator_ << ",\n";
    out << "  \"remaining_males\": " << remaining_males_ << ",\n";
    out << "  \"remaining_females\": " << remaining_females_ << ",\n";
    out << "  \"resource_spawn_timer\": " << resource_spawn_timer_ << ",\n";
    out << "  \"status\": " << static_cast<int>(status_) << ",\n";
    out << "  \"next_id\": " << entity_mgr_.GetNextId() << ",\n";

    // Population history
    out << "  \"history\": [\n";
    for (size_t i = 0; i < population_history_.size(); ++i) {
        out << "    {\"males\": " << population_history_[i].males
            << ", \"females\": " << population_history_[i].females
            << ", \"food\": " << population_history_[i].food << "}"
            << (i + 1 < population_history_.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    // Resources
    out << "  \"resources\": [\n";
    const auto& res_list = entity_mgr_.GetResources();
    for (size_t i = 0; i < res_list.size(); ++i) {
        const auto& res = res_list[i];
        out << "    {"
            << "\"id\": " << res.id << ", "
            << "\"x\": " << res.pos.x << ", "
            << "\"y\": " << res.pos.y << ", "
            << "\"type\": " << static_cast<int>(res.type) << ", "
            << "\"calories\": " << res.calories << ", "
            << "\"scent\": " << res.scent << ", "
            << "\"alive\": " << (res.alive ? "true" : "false")
            << "}" << (i + 1 < res_list.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    // Organisms
    out << "  \"organisms\": [\n";
    const auto& org_list = entity_mgr_.GetOrganisms();
    for (size_t i = 0; i < org_list.size(); ++i) {
        const auto& org = org_list[i];
        out << "    {"
            << "\"id\": " << org.id << ", "
            << "\"x\": " << org.pos.x << ", "
            << "\"y\": " << org.pos.y << ", "
            << "\"w\": " << org.w << ", "
            << "\"h\": " << org.h << ", "
            << "\"gender\": " << static_cast<int>(org.gender) << ", "
            << "\"life_force\": " << org.life_force << ", "
            << "\"hunger\": " << org.hunger << ", "
            << "\"thirst\": " << org.thirst << ", "
            << "\"calories\": " << org.calories << ", "
            << "\"lifetime_calories\": " << org.lifetime_calories << ", "
            << "\"age\": " << org.age << ", "
            << "\"scent\": " << org.scent << ", "
            << "\"sight_range\": " << org.sight_range << ", "
            << "\"smell_range\": " << org.smell_range << ", "
            << "\"damage\": " << org.damage << ", "
            << "\"gestating\": " << (org.gestating ? "true" : "false") << ", "
            << "\"gestation_timer\": " << org.gestation_timer << ", "
            << "\"father_id\": " << org.father_id << ", "
            << "\"father_scent\": " << org.father_scent << ", "
            << "\"father_sight_range\": " << org.father_sight_range << ", "
            << "\"father_smell_range\": " << org.father_smell_range << ", "
            << "\"age_timer\": " << org.age_timer << ", "
            << "\"step_cooldown\": " << org.step_cooldown << ", "
            << "\"current_state\": " << static_cast<int>(org.current_state) << ", "
            << "\"alive\": " << (org.alive ? "true" : "false")
            << "}" << (i + 1 < org_list.size() ? ",\n" : "\n");
    }
    out << "  ]\n";
    out << "}\n";

    return out.good();
}

bool Simulation::LoadState(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();

    JsonValue root;
    if (!JsonParser::Parse(content, root)) {
        return false;
    }

    if (!root.contains("version")) {
        return false;
    }

    grid_.Clear();
    entity_mgr_.Clear();

    step_count_ = root["step_count"].as_uint64();
    step_accumulator_ = root["step_accumulator"].as_float();
    remaining_males_ = root["remaining_males"].as_int();
    remaining_females_ = root["remaining_females"].as_int();
    resource_spawn_timer_ = root["resource_spawn_timer"].as_float();
    status_ = static_cast<GameStatus>(root["status"].as_int());

    uint32_t saved_next_id = root["next_id"].as_uint32();
    entity_mgr_.SetNextId(saved_next_id > 0 ? saved_next_id : 1);

    population_history_.clear();
    const auto& hist_arr = root["history"].arr_val;
    for (const auto& sample_val : hist_arr) {
        population_history_.push_back({
            sample_val["males"].as_int(),
            sample_val["females"].as_int(),
            sample_val["food"].as_int()
        });
    }

    const auto& res_arr = root["resources"].arr_val;
    for (const auto& r_val : res_arr) {
        Resource res;
        res.id = r_val["id"].as_uint32();
        res.pos = {r_val["x"].as_int(), r_val["y"].as_int()};
        res.type = static_cast<EntityType>(r_val["type"].as_int());
        res.calories = r_val["calories"].as_int();
        res.scent = static_cast<uint16_t>(r_val["scent"].as_int());
        res.alive = r_val["alive"].as_bool();
        entity_mgr_.AddResource(res);
        if (res.alive) {
            grid_.Occupy(res.pos.x, res.pos.y, 1, 1, res.type, res.id);
        }
    }

    const auto& org_arr = root["organisms"].arr_val;
    for (const auto& o_val : org_arr) {
        Organism org;
        org.id = o_val["id"].as_uint32();
        org.pos = {o_val["x"].as_int(), o_val["y"].as_int()};
        org.w = o_val["w"].as_int();
        org.h = o_val["h"].as_int();
        org.gender = static_cast<Gender>(o_val["gender"].as_int());
        org.life_force = o_val["life_force"].as_float();
        org.hunger = o_val["hunger"].as_float();
        org.thirst = o_val["thirst"].as_float();
        org.calories = o_val["calories"].as_int();
        org.lifetime_calories = o_val["lifetime_calories"].as_int();
        org.age = o_val["age"].as_int();
        org.scent = static_cast<uint16_t>(o_val["scent"].as_int());
        org.sight_range = o_val["sight_range"].as_int();
        org.smell_range = o_val["smell_range"].as_int();
        org.damage = o_val["damage"].as_int();
        org.gestating = o_val["gestating"].as_bool();
        org.gestation_timer = o_val["gestation_timer"].as_float();
        org.father_id = o_val["father_id"].as_uint32();
        org.father_scent = static_cast<uint16_t>(o_val["father_scent"].as_int());
        org.father_sight_range = o_val["father_sight_range"].as_int();
        org.father_smell_range = o_val["father_smell_range"].as_int();
        org.age_timer = o_val["age_timer"].as_float();
        org.step_cooldown = o_val["step_cooldown"].as_float();
        org.current_state = static_cast<FSMState>(o_val["current_state"].as_int());
        org.alive = o_val["alive"].as_bool();

        entity_mgr_.AddOrganism(org);
        if (org.alive) {
            grid_.Occupy(org.pos.x, org.pos.y, org.w, org.h, EntityType::ORGANISM, org.id);
        }
    }

    return true;
}

} // namespace sim

