#pragma once

#include "sim/Entity.hpp"
#include "sim/GridMatrix.hpp"
#include <vector>
#include <cstdint>
#include <optional>

namespace sim {

class EntityManager {
public:
    EntityManager();

    uint32_t CreateResource(Vector2i pos, EntityType type);
    uint32_t CreateOrganism(Vector2i pos, Gender gender, int w = 1, int h = 1, int calories = 40, uint16_t scent = 0);

    Organism* GetOrganism(uint32_t id) noexcept;
    const Organism* GetOrganism(uint32_t id) const noexcept;

    Resource* GetResource(uint32_t id) noexcept;
    const Resource* GetResource(uint32_t id) const noexcept;

    void RemoveDeadEntities(GridMatrix& grid);

    [[nodiscard]] std::vector<Organism>& GetOrganisms() noexcept { return organisms_; }
    [[nodiscard]] const std::vector<Organism>& GetOrganisms() const noexcept { return organisms_; }

    [[nodiscard]] std::vector<Resource>& GetResources() noexcept { return resources_; }
    [[nodiscard]] const std::vector<Resource>& GetResources() const noexcept { return resources_; }

    [[nodiscard]] size_t ActiveOrganismCount() const noexcept;
    [[nodiscard]] size_t ActiveResourceCount() const noexcept;

    void Clear();

private:
    std::vector<Organism> organisms_;
    std::vector<Resource> resources_;
    uint32_t next_id_{1};
};

} // namespace sim

