#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace sim {

constexpr int GRID_WIDTH = 100;
constexpr int GRID_HEIGHT = 100;
constexpr int CELL_SIZE_PX = 10;
constexpr int PLAYFIELD_WIDTH_PX = GRID_WIDTH * CELL_SIZE_PX;   // 1000
constexpr int PLAYFIELD_HEIGHT_PX = GRID_HEIGHT * CELL_SIZE_PX; // 1000
constexpr int SIDEBAR_WIDTH_PX = 280;
constexpr int WINDOW_WIDTH_PX = PLAYFIELD_WIDTH_PX + SIDEBAR_WIDTH_PX; // 1280
constexpr int WINDOW_HEIGHT_PX = PLAYFIELD_HEIGHT_PX;                  // 1000

constexpr float BASE_STEP_INTERVAL = 1.0f; // 1 cell per second
constexpr float AGING_INTERVAL = 360.0f;   // 6 minutes
constexpr float GESTATION_TIME = 90.0f;    // 90 seconds
constexpr int INITIAL_FOOD_COUNT = 250;
constexpr int INITIAL_WATER_COUNT = 250;
constexpr float MIN_RESOURCE_SPAWN_INTERVAL = 5.0f;
constexpr float MAX_RESOURCE_SPAWN_INTERVAL = 15.0f;

constexpr float THIRST_DEPLETION_RATE = 0.10f; // per second
constexpr float HUNGER_DEPLETION_RATE = 0.05f; // per second
constexpr float STARVATION_DAMAGE_RATE = 2.0f; // per second
constexpr float PASSIVE_HEAL_THRESHOLD = 5.0f; // > 50% hunger & thirst
constexpr float PASSIVE_HEAL_RATE = 1.0f;      // +1.0 life force per second
constexpr float MIN_FOOD_HEAL = 5.0f;          // min food health burst
constexpr float MAX_FOOD_HEAL = 15.0f;         // max food health burst
constexpr int CALORIE_MOVE_COST = 1;
constexpr int FOOD_CALORIES = 40;
constexpr float CRITICAL_NEED_THRESHOLD = 3.0f;
constexpr float REPRODUCTION_MIN_HUNGER = 6.0f;
constexpr float REPRODUCTION_MIN_THIRST = 6.0f;
constexpr int REPRODUCTION_MIN_AGE = 2;

constexpr int MAX_MANUAL_MALES = 10;
constexpr int MAX_MANUAL_FEMALES = 10;
constexpr size_t VICTORY_CELL_THRESHOLD = 7500; // 75% of 10,000

constexpr uint16_t SCENT_FOOD = 0xFFFE;
constexpr uint16_t SCENT_WATER = 0xFFFF;

// Resource Presentation Colors (50% Darkened: Deep Red for Food, Medium Slate/Gray for Water
// to ensure brighter organism entities stand out on the playfield)
constexpr uint32_t COLOR_FOOD_RGB = 0x761C1C;
constexpr uint32_t COLOR_WATER_RGB = 0x7D7D80;

struct Vector2i {
  int x{0};
  int y{0};

  constexpr bool operator==(const Vector2i &other) const noexcept = default;
  constexpr Vector2i operator+(const Vector2i &other) const noexcept {
    return {x + other.x, y + other.y};
  }
  constexpr Vector2i operator-(const Vector2i &other) const noexcept {
    return {x - other.x, y - other.y};
  }
};

enum class EntityType : uint8_t { EMPTY = 0, FOOD, WATER, ORGANISM };

enum class Gender : uint8_t { MALE = 0, FEMALE };

enum class FSMState : uint8_t { WANDER = 0, CRITICAL_NEED, SEEK_MATE };

enum class GameStatus : uint8_t { ONGOING = 0, VICTORY, DEFEAT };

enum class ActiveTool : uint8_t { FOOD = 1, WATER = 2, MALE = 3, FEMALE = 4 };

} // namespace sim
