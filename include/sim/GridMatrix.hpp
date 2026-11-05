#pragma once

#include "sim/Common.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sim {

struct Cell {
    EntityType type{EntityType::EMPTY};
    uint32_t entity_id{0}; // 0 = empty / unassigned
};

class GridMatrix {
public:
    GridMatrix();

    void Clear();

    [[nodiscard]] bool InBounds(int x, int y) const noexcept;
    [[nodiscard]] bool CanPlace(int x, int y, int w, int h, uint32_t ignore_id = 0) const noexcept;
    void Occupy(int x, int y, int w, int h, EntityType type, uint32_t entity_id);
    void Vacate(int x, int y, int w, int h);

    [[nodiscard]] bool CanExpand(int x, int y, int current_w, int current_h, int target_w, int target_h, uint32_t entity_id) const noexcept;

    [[nodiscard]] const Cell& At(int x, int y) const;
    [[nodiscard]] Cell& At(int x, int y);

    [[nodiscard]] size_t OccupiedCellCount() const noexcept { return occupied_count_; }

private:
    std::array<std::array<Cell, GRID_HEIGHT>, GRID_WIDTH> cells_{};
    size_t occupied_count_{0};
};

} // namespace sim

