#include "sim/GridMatrix.hpp"
#include <cassert>

namespace sim {

GridMatrix::GridMatrix() {
    Clear();
}

void GridMatrix::Clear() {
    for (int x = 0; x < GRID_WIDTH; ++x) {
        for (int y = 0; y < GRID_HEIGHT; ++y) {
            cells_[x][y] = Cell{EntityType::EMPTY, 0};
        }
    }
    occupied_count_ = 0;
}

bool GridMatrix::InBounds(int x, int y) const noexcept {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

bool GridMatrix::CanPlace(int x, int y, int w, int h, uint32_t ignore_id) const noexcept {
    if (w <= 0 || h <= 0) return false;
    if (x < 0 || y < 0 || (x + w) > GRID_WIDTH || (y + h) > GRID_HEIGHT) {
        return false;
    }

    for (int cx = x; cx < x + w; ++cx) {
        for (int cy = y; cy < y + h; ++cy) {
            const Cell& c = cells_[cx][cy];
            if (c.type != EntityType::EMPTY && (ignore_id == 0 || c.entity_id != ignore_id)) {
                return false;
            }
        }
    }
    return true;
}

void GridMatrix::Occupy(int x, int y, int w, int h, EntityType type, uint32_t entity_id) {
    assert(CanPlace(x, y, w, h, entity_id));
    for (int cx = x; cx < x + w; ++cx) {
        for (int cy = y; cy < y + h; ++cy) {
            if (type == EntityType::ORGANISM && cells_[cx][cy].type != EntityType::ORGANISM) {
                ++occupied_count_;
            }
            cells_[cx][cy] = Cell{type, entity_id};
        }
    }
}

void GridMatrix::Vacate(int x, int y, int w, int h) {
    if (!InBounds(x, y)) return;
    int max_x = std::min(x + w, GRID_WIDTH);
    int max_y = std::min(y + h, GRID_HEIGHT);

    for (int cx = x; cx < max_x; ++cx) {
        for (int cy = y; cy < max_y; ++cy) {
            if (cells_[cx][cy].type == EntityType::ORGANISM) {
                if (occupied_count_ > 0) {
                    --occupied_count_;
                }
            }
            cells_[cx][cy] = Cell{EntityType::EMPTY, 0};
        }
    }
}

bool GridMatrix::CanExpand(int x, int y, int /*current_w*/, int /*current_h*/, int target_w, int target_h, uint32_t entity_id) const noexcept {
    return CanPlace(x, y, target_w, target_h, entity_id);
}

const Cell& GridMatrix::At(int x, int y) const {
    assert(InBounds(x, y));
    return cells_[x][y];
}

Cell& GridMatrix::At(int x, int y) {
    assert(InBounds(x, y));
    return cells_[x][y];
}

} // namespace sim

