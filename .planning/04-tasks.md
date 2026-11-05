# Work Breakdown Structure & Atomic Tasks
## Autonomous 2D Multi-Tile Organism Simulation (A-Life)

- **Document ID:** TASKS-01
- **File Target:** `.planning/04-tasks.md`
- **Dependency Flow:**
  ```
  TASK-01 (CMake & Scaffolding)
     │
     ▼
  TASK-02 (Common Types & Math)
     │
     ▼
  TASK-03 (GridMatrix Spatial Collision) ──► TASK-04 (Headless Test Harness & Grid Tests)
     │                                            │
     ▼                                            ▼
  TASK-05 (EntityManager Data Model) ────────► (Integrate into test runner)
     │
     ▼
  TASK-06 (Metabolism & Resource Engine) ────► (Metabolism unit tests)
     │
     ▼
  TASK-07 (Chebyshev Metrics & AI FSM) ──────► (AI & FSM integration tests)
     │
     ▼
  TASK-08 (Simulation Controller & Rules) ───► (Rules & Quotas validation)
     │
     ▼
  TASK-09 (Raylib Playfield Renderer)
     │
     ▼
  TASK-10 (Sidebar HUD & Interactive App)
  ```

---

### [TASK-01]: CMake Build System & Project Scaffolding

- **Prerequisites:** None.
- **Target Files:**
  - Create: `CMakeLists.txt`
  - Create: Directories `include/sim`, `src/sim`, `include/app`, `src/app`, `tests`
- **Scope & Objective:** Establish standard C++20 CMake configuration fetching Raylib 5.0 with `CMAKE_POLICY_VERSION_MINIMUM=3.5` compatibility. Define targets for `sim_core` (static library), `sim_tests` (headless test executable), and `organism_sim` (Raylib GUI executable).
- **Interface / Contract:**
  ```cmake
  cmake_minimum_required(VERSION 3.22)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
  project(organism_sim C CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  # Compiler warnings: -Wall -Wextra -pedantic
  # Targets:
  #   sim_core (STATIC) -> src/sim/*.cpp
  #   sim_tests (EXECUTABLE) -> tests/*.cpp, links sim_core
  #   organism_sim (EXECUTABLE) -> src/app/*.cpp, links sim_core, raylib
  ```
- **Behavior & Edge Cases:**
  - CMake 4.x policy suppression prevents legacy version deprecation failure.
  - Builds cleanly with sanitizers enabled via `-DENABLE_SANITIZERS=ON`.
- **Verification & Acceptance Criteria:**
  - [ ] `cmake -B build` completes without errors or warnings.
  - [ ] Execution verification command:
    ```bash
    cmake -B build -DENABLE_SANITIZERS=ON && cmake --build build
    ```

---

### [TASK-02]: Core Types, Constants & Vector Math

- **Prerequisites:** TASK-01.
- **Target Files:**
  - Create: `include/sim/Common.hpp`
- **Scope & Objective:** Define universal simulation constants, grid dimensions ($100 \times 100$), discrete coordinate struct `Vector2i`, entity enumerations, and mathematical helpers. Out of scope: dynamic entity storage or Raylib data types.
- **Interface / Contract:**
  ```cpp
  namespace sim {
      constexpr int GRID_WIDTH = 100;
      constexpr int GRID_HEIGHT = 100;
      constexpr int CELL_SIZE_PX = 10;
      constexpr float TICK_INTERVAL_BASE = 1.0f;
      constexpr uint16_t SCENT_FOOD = 0xFFFE;
      constexpr uint16_t SCENT_WATER = 0xFFFF;

      struct Vector2i {
          int x{0};
          int y{0};
          constexpr bool operator==(const Vector2i& other) const noexcept = default;
          constexpr Vector2i operator+(const Vector2i& other) const noexcept { return {x + other.x, y + other.y}; }
          constexpr Vector2i operator-(const Vector2i& other) const noexcept { return {x - other.x, y - other.y}; }
      };

      enum class EntityType : uint8_t { EMPTY = 0, FOOD, WATER, ORGANISM };
      enum class Gender : uint8_t { MALE = 0, FEMALE };
      enum class FSMState : uint8_t { WANDER = 0, SEEK_FOOD, SEEK_WATER, SEEK_MATE };
      enum class GameStatus : uint8_t { ONGOING = 0, VICTORY, DEFEAT };
  }
  ```
- **Behavior & Edge Cases:**
  - Header-only, constexpr-friendly arithmetic. Zero external runtime dependencies.
- **Verification & Acceptance Criteria:**
  - [ ] Compiles cleanly under `sim_core` target.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-03]: Spatial Grid & Multi-Tile Collision Engine (`GridMatrix`)

- **Prerequisites:** TASK-02.
- **Target Files:**
  - Create: `include/sim/GridMatrix.hpp`
  - Create: `src/sim/GridMatrix.cpp`
- **Scope & Objective:** Implement the $100 \times 100$ spatial occupancy matrix supporting rectangular footprints up to $4 \times 4$. Provide collision checking, placement, vacation, in-place growth validation, and total occupied cell tracking.
- **Interface / Contract:**
  ```cpp
  namespace sim {
      struct Cell {
          EntityType type{EntityType::EMPTY};
          uint32_t entity_id{0}; // 0 = empty
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
          [[nodiscard]] size_t OccupiedCellCount() const noexcept;
      private:
          std::array<std::array<Cell, GRID_HEIGHT>, GRID_WIDTH> cells_;
          size_t occupied_count_{0};
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Boundary rejection: coordinates $<0$ or $x + w > 100$ return `false` without memory violation.
  - `CanExpand` validates only newly added perimeter cells while treating current organism cells as valid.
  - Occupied cell counter increments and decrements in $O(1)$.
- **Verification & Acceptance Criteria:**
  - [ ] Builds within `sim_core`.
  - [ ] Passes contract invariants.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-04]: Headless Test Runner & Grid Collision Suite

- **Prerequisites:** TASK-03.
- **Target Files:**
  - Create: `tests/test_main.cpp`
  - Create: `tests/test_grid.cpp`
  - Modify: `CMakeLists.txt`
- **Scope & Objective:** Build lightweight, zero-dependency test runner macro harness (`TEST_CASE`, `CHECK_TRUE`, `CHECK_EQ`). Implement verification test cases `TC-GRID-01` through `TC-GRID-10` from the test matrix.
- **Interface / Contract:**
  ```cpp
  // tests/test_main.cpp
  int main(int argc, char** argv);
  // test registration macros
  ```
- **Behavior & Edge Cases:**
  - Returns exit code 0 on complete pass, 1 on any assertion failure.
  - Runs in pure headless terminal environment without display server.
- **Verification & Acceptance Criteria:**
  - [ ] All 10 Grid test cases pass.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/tests/sim_tests
    ```

---

### [TASK-05]: Entity Data Model & Contiguous `EntityManager`

- **Prerequisites:** TASK-02, TASK-04.
- **Target Files:**
  - Create: `include/sim/Entity.hpp`
  - Create: `include/sim/EntityManager.hpp`
  - Create: `src/sim/EntityManager.cpp`
- **Scope & Objective:** Implement dense contiguous storage for `Organism` and `Resource` instances. Support incremental ID generation, lookup by ID, active filtering, and swap-and-pop removal for dead entities.
- **Interface / Contract:**
  ```cpp
  namespace sim {
      struct Resource {
          uint32_t id{0};
          Vector2i pos{0, 0};
          EntityType type{EntityType::FOOD};
          int calories{0};
          uint16_t scent{0};
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
          uint16_t scent{0};
          int sight_range{4};
          int smell_range{6};
          bool gestating{false};
          float gestation_timer{0.0f};
          float age_timer{0.0f};
          float step_cooldown{0.0f};
          FSMState current_state{FSMState::WANDER};
          bool alive{true};
      };

      class EntityManager {
      public:
          uint32_t CreateResource(Vector2i pos, EntityType type);
          uint32_t CreateOrganism(Vector2i pos, Gender gender, int w = 1, int h = 1);
          Organism* GetOrganism(uint32_t id);
          Resource* GetResource(uint32_t id);
          void RemoveDeadEntities(GridMatrix& grid);
          std::vector<Organism>& GetOrganisms() noexcept;
          std::vector<Resource>& GetResources() noexcept;
          void Clear();
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Dynamic ID sequence starts at 1 (0 reserved for EMPTY).
  - Dead entity removal cleans up both the vector storage and grid cells cleanly without invalidating iterating loops.
- **Verification & Acceptance Criteria:**
  - [ ] Compiles cleanly into `sim_core`.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-06]: Metabolism, Aging & Resource Engine

- **Prerequisites:** TASK-05.
- **Target Files:**
  - Create: `include/sim/Simulation.hpp`
  - Create: `src/sim/Simulation.cpp`
  - Create: `tests/test_metabolism.cpp`
- **Scope & Objective:** Implement metabolic consumption logic: thirst drain ($0.10/\text{s}$), hunger drain ($0.05/\text{s}$), calorie expenditure, starvation/dehydration damage ($2.0/\text{s}$), aging ($360\text{s}$), resource ingestion, and random periodic resource spawning ($[60\text{s}, 120\text{s}]$).
- **Interface / Contract:**
  ```cpp
  namespace sim {
      void UpdateMetabolism(Organism& org, float dt);
      void ConsumeResource(Organism& org, Resource& res);
      void UpdateAging(Organism& org, float dt);
  }
  ```
- **Behavior & Edge Cases:**
  - Starvation damage applies whenever `hunger <= 0.0f` or `thirst <= 0.0f`.
  - Exhaustion penalty: `calories == 0` causes $1.5\times$ hunger decay.
  - Consuming food restores hunger to $10.0$ and adds $40$ calories; consuming water restores thirst to $10.0$.
- **Verification & Acceptance Criteria:**
  - [ ] Unit tests in `test_metabolism.cpp` pass `TC-MET-01` through `TC-MET-11`.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/tests/sim_tests
    ```

---

### [TASK-07]: Chebyshev Distance, AI Sensory & Priority FSM

- **Prerequisites:** TASK-06.
- **Target Files:**
  - Create: `include/sim/AIEngine.hpp`
  - Create: `src/sim/AIEngine.cpp`
  - Create: `tests/test_ai_fsm.cpp`
- **Scope & Objective:** Implement $O(1)$ Chebyshev bounding-box distance calculation between multi-tile entities. Implement olfactory and visual target scanning, priority state machine (Critical Need > Mating > Wander), gestation/offspring generation with clamped traits, and greedy movement step with 8-way obstacle fallback.
- **Interface / Contract:**
  ```cpp
  namespace sim {
      class AIEngine {
      public:
          static int ChebyshevDistance(const Vector2i& pos_a, int w_a, int h_a, const Vector2i& pos_b, int w_b, int h_b) noexcept;
          static bool AreAdjacent(const Vector2i& pos_a, int w_a, int h_a, const Vector2i& pos_b, int w_b, int h_b) noexcept;
          static bool AreScentsCompatible(uint16_t scent_a, uint16_t scent_b) noexcept;
          void UpdateAI(Organism& org, EntityManager& entity_mgr, GridMatrix& grid, float dt, std::mt19937& rng);
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Bounding-box distance handles differing sizes ($1 \times 1$ to $4 \times 4$).
  - Critical hunger/thirst ($\le 3.0$) strictly preempts mating.
  - Scent compatibility requires `abs(scent_a - scent_b) % 10 < 7`.
  - Offspring inherits clamped ranges ($S \in [2,6]$, $M \in [4,10]$, $W=1, H=1$, calories $[20, 60]$).
  - Gestation deferral: if female has no adjacent empty cell upon 90s completion, birth is deferred until space is available.
- **Verification & Acceptance Criteria:**
  - [ ] Unit tests in `test_ai_fsm.cpp` pass `TC-AI-01` through `TC-AI-08`.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/tests/sim_tests
    ```

---

### [TASK-08]: Simulation Controller, Game Status & Quota Rules

- **Prerequisites:** TASK-07.
- **Target Files:**
  - Modify: `include/sim/Simulation.hpp`
  - Modify: `src/sim/Simulation.cpp`
  - Create: `tests/test_rules.cpp`
- **Scope & Objective:** Unify the simulation controller orchestrating grid, entities, metabolism, AI, and periodic resource spawning. Enforce manual placement limits (10 Male, 10 Female, unlimited Food/Water), evaluate Victory ($\ge 75\%$ coverage) and Loss ($0$ organisms), and provide deterministic `Reset()`.
- **Interface / Contract:**
  ```cpp
  namespace sim {
      class Simulation {
      public:
          Simulation(uint32_t seed = 42);
          void Reset();
          void Step(float dt);
          bool ManualPlace(int cell_x, int cell_y, EntityType type, Gender gender = Gender::MALE);
          [[nodiscard]] int RemainingMalePlacements() const noexcept;
          [[nodiscard]] int RemainingFemalePlacements() const noexcept;
          [[nodiscard]] GameStatus GetGameStatus() const noexcept;
          [[nodiscard]] float OccupancyPercentage() const noexcept;
          [[nodiscard]] const GridMatrix& GetGrid() const noexcept;
          [[nodiscard]] const EntityManager& GetEntities() const noexcept;
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Manual placement rejects when placement quotas are exhausted ($0$ remaining).
  - Initial setup seeds exactly 20 Food, 20 Water, 1 Male, and 1 Female.
  - Victory triggers immediately once footprint cells $\ge 7,500$.
- **Verification & Acceptance Criteria:**
  - [ ] Unit tests in `test_rules.cpp` pass `TC-RULE-01` through `TC-RULE-07`.
  - [ ] All test suites in `sim_tests` pass 100% cleanly.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/tests/sim_tests
    ```

---

### [TASK-09]: Raylib Playfield Renderer

- **Prerequisites:** TASK-08.
- **Target Files:**
  - Create: `include/app/Renderer.hpp`
  - Create: `src/app/Renderer.cpp`
- **Scope & Objective:** Implement Raylib-based rendering of the $1000 \times 1000$ playfield. Render subtle grid lines, food (green tiles), water (cyan tiles), and organisms with multi-tile bounding rectangles, shaded by gender (Blue/Yellow) and darkened/desaturated based on health (`life_force`).
- **Interface / Contract:**
  ```cpp
  namespace app {
      class Renderer {
      public:
          void RenderPlayfield(const sim::Simulation& sim);
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Multi-tile organisms rendered as unified bounding rectangles with interior shading and border highlights.
  - Dying organisms fade toward dark gray as `life_force` approaches 0.
- **Verification & Acceptance Criteria:**
  - [ ] Compiles cleanly and links against `raylib`.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-10]: Sidebar HUD, Input Controls & Application Main

- **Prerequisites:** TASK-09.
- **Target Files:**
  - Create: `include/app/UI.hpp`
  - Create: `src/app/UI.cpp`
  - Create: `src/app/main.cpp`
- **Scope & Objective:** Build the $280 \times 1000$ sidebar HUD and interactive event loop in `main.cpp`. Implement tool selection (`1`: Food, `2`: Water, `3`: Male, `4`: Female), mouse click placement, pause toggle (`Space`), speed multiplier (`+` / `-`), and reset (`R`). Display occupancy %, population counters, and victory/defeat banners.
- **Interface / Contract:**
  ```cpp
  namespace app {
      class UI {
      public:
          void RenderSidebar(const sim::Simulation& sim, int active_tool, float time_scale, bool paused);
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Mouse clicks on sidebar do not trigger placements on playfield.
  - Mouse click on occupied cell produces visual feedback without corrupting grid.
  - Time scaling advances simulation by fixed timesteps ($1\times, 2\times, 4\times, 8\times$).
- **Verification & Acceptance Criteria:**
  - [ ] Window opens at $1280 \times 1000$ at 60 FPS.
  - [ ] Complete Milestone 1 through Milestone 4 acceptance criteria satisfied.
  - [ ] Execution verification command:
    ```bash
    cmake --build build && ./build/organism_sim
    ```

---

### [TASK-11]: 80-Resource Baseline & Proportional Spawning

- **Prerequisites:** TASK-10.
- **Target Files:**
  - Modify: `include/sim/Common.hpp`
  - Modify: `src/sim/Simulation.cpp`
- **Scope & Objective:** Increase the initial food and water seeding count from 20 to 80 each (total 160 initial resource units). Scale the periodic background spawn interval down to $[20\text{s}, 40\text{s}]$ to sustain the larger ecosystem.
- **Interface / Contract:**
  ```cpp
  constexpr int INITIAL_FOOD_COUNT = 80;
  constexpr int INITIAL_WATER_COUNT = 80;
  constexpr float MIN_RESOURCE_SPAWN_INTERVAL = 20.0f;
  constexpr float MAX_RESOURCE_SPAWN_INTERVAL = 40.0f;
  ```
- **Behavior & Edge Cases:**
  - `SeedInitialWorld()` places 80 food and 80 water on unoccupied cells without overlap.
  - `Reset()` restores the grid to 80 food, 80 water, 1 male, and 1 female.
- **Verification & Acceptance Criteria:**
  - [ ] Compiles cleanly into `sim_core`.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-12]: Hover Organism Query Contract

- **Prerequisites:** TASK-11.
- **Target Files:**
  - Modify: `include/sim/Simulation.hpp`
  - Modify: `src/sim/Simulation.cpp`
- **Scope & Objective:** Expose a thread-safe, boundary-guarded spatial lookup method `GetOrganismAt(int cell_x, int cell_y)` on `Simulation` that maps any grid coordinate to its occupying organism handle, returning `nullptr` if empty, occupied by food/water, or out-of-bounds.
- **Interface / Contract:**
  ```cpp
  namespace sim {
      class Simulation {
      public:
          [[nodiscard]] const Organism* GetOrganismAt(int cell_x, int cell_y) const noexcept;
      };
  }
  ```
- **Behavior & Edge Cases:**
  - OOB coordinates ($x < 0, y < 0, x \ge 100, y \ge 100$) return `nullptr` without assertion crashes.
  - Multi-tile organisms ($W \times H$) resolve correctly from any of their covered footprint cells.
- **Verification & Acceptance Criteria:**
  - [ ] Compiles cleanly into `sim_core`.
  - [ ] Passes spatial lookup contract invariants.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-13]: Sidebar Bottom Inspector Card Component

- **Prerequisites:** TASK-12.
- **Target Files:**
  - Modify: `include/app/UI.hpp`
  - Modify: `src/app/UI.cpp`
  - Modify: `src/app/main.cpp`
- **Scope & Objective:** Extend the sidebar HUD ($280 \times 1000$) to render a detailed telemetry inspection card in the unused bottom space below the controls section ($Y \approx 620-980$ px). Pass hovered cell coordinate from `main.cpp` into `UI::DrawSidebar`.
- **Interface / Contract:**
  ```cpp
  namespace app {
      class UI {
      public:
          void DrawSidebar(const sim::Simulation& sim, sim::ActiveTool active_tool, float time_scale, bool paused, const sim::Organism* hovered_org);
      };
  }
  ```
- **Behavior & Edge Cases:**
  - Displays ID, Gender, Size $W \times H$, Damage, Health Bar ($0-100$), Hunger/Thirst ratings, Calories, Age, Scent, State, and Gestation countdown.
  - If no organism is hovered, displays placeholder prompt `[Hover over organism to inspect stats]`.
- **Verification & Acceptance Criteria:**
  - [ ] Builds and links against `raylib`.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-14]: Automated Headless Verification Suite for 80 Resources & Inspector

- **Prerequisites:** TASK-13.
- **Target Files:**
  - Modify: `tests/test_rules.cpp`
- **Scope & Objective:** Implement automated test assertions validating `TC-RULE-09` (80 Food, 80 Water seeding) and `TC-INSP-01` through `TC-INSP-05` (empty cell, resource cell, 1x1 organism, multi-tile footprint, OOB coordinates).
- **Behavior & Edge Cases:**
  - Confirms zero assertion failures under AddressSanitizer and UndefinedBehaviorSanitizer.
- **Verification & Acceptance Criteria:**
  - [ ] All test cases in `sim_tests` pass 100% cleanly.
  - [ ] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/sim_tests
    ```

---

### [TASK-15]: Passive Healing Constants & Metabolism Engine Update

- **Prerequisites:** TASK-14.
- **Target Files:**
  - Modify: `include/sim/Common.hpp`
  - Modify: `src/sim/Simulation.cpp`
- **Scope & Objective:** Define passive healing constants in `Common.hpp` (`PASSIVE_HEAL_THRESHOLD = 5.0f`, `PASSIVE_HEAL_RATE = 1.0f`). In `Simulation::UpdateMetabolismAndGrowth`, steadily restore organism life force by $1.0\text{f}/\text{s}$ whenever hunger and thirst both exceed 5.0 ($>50\%$), clamping at $100.0\text{f}$.
- **Interface / Contract:**
  ```cpp
  constexpr float PASSIVE_HEAL_THRESHOLD = 5.0f; // > 50%
  constexpr float PASSIVE_HEAL_RATE = 1.0f;      // life force / second
  ```
- **Behavior & Edge Cases:**
  - Zero passive healing applies if either `hunger <= 5.0f` or `thirst <= 5.0f`.
  - Max health clamped cleanly to $100.0\text{f}$ without floating-point overflow.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly into `sim_core`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-16]: Nutritional Food Healing Burst on Consumption

- **Prerequisites:** TASK-15.
- **Target Files:**
  - Modify: `include/sim/Common.hpp`
  - Modify: `src/sim/AIEngine.cpp`
- **Scope & Objective:** Add food healing constants (`MIN_FOOD_HEAL = 5.0f`, `MAX_FOOD_HEAL = 15.0f`). When an organism consumes food in `AIEngine`, generate a uniform random healing burst in $[5.0\text{f}, 15.0\text{f}]$ and add it to `life_force`, clamped to $100.0\text{f}$.
- **Interface / Contract:**
  ```cpp
  constexpr float MIN_FOOD_HEAL = 5.0f;
  constexpr float MAX_FOOD_HEAL = 15.0f;
  ```
- **Behavior & Edge Cases:**
  - Consuming water continues to restore only thirst without triggering health bursts.
  - Clamps to $100.0\text{f}$ if initial life force was already $> 85.0\text{f}$.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly into `sim_core`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-17]: Automated Verification Suite for Dual Healing Mechanics

- **Prerequisites:** TASK-16.
- **Target Files:**
  - Modify: `tests/test_metabolism.cpp`
- **Scope & Objective:** Implement automated headless unit tests verifying `TC-MET-12` (passive healing active), `TC-MET-13` (threshold boundary at 5.0), `TC-MET-14` (passive max clamp), `TC-MET-15` (nutritional food burst), and `TC-MET-16` (food burst max clamp).
- **Behavior & Edge Cases:**
  - Confirms zero assertion failures under AddressSanitizer and UndefinedBehaviorSanitizer.
- **Verification & Acceptance Criteria:**
  - [x] All test cases in `sim_tests` pass 100% cleanly.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/sim_tests
    ```

---

### [TASK-26]: Custom Clean and Rebuild Targets in CMakeLists.txt

- **Prerequisites:** TASK-25.
- **Target Files:**
  - Modify: `CMakeLists.txt`
- **Scope & Objective:** Implement custom targets `clean_all` (invoking the clean target on the build directory) and `rebuild` (clean followed by building `organism_sim` and `sim_tests`) to enhance developer workflow usability.
- **Interface / Contract:**
  ```cmake
  add_custom_target(clean_all
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
      COMMENT "Cleaning all project targets..."
  )
  add_custom_target(rebuild
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target organism_sim
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target sim_tests
      COMMENT "Executing full clean and rebuild..."
  )
  ```
- **Behavior & Edge Cases:**
  - Operates cleanly on both Ninja and Unix Makefiles generators without hardcoded shell calls.
- **Verification & Acceptance Criteria:**
  - [x] `cmake --build build --target clean_all` executes cleanly.
  - [x] `cmake --build build --target rebuild` completes successfully.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target clean_all && cmake --build build --target rebuild
    ```

---

### [TASK-27]: GNUInstallDirs Installation Target for Executable, Desktop Launcher & Icons

- **Prerequisites:** TASK-26.
- **Target Files:**
  - Modify: `CMakeLists.txt`
- **Scope & Objective:** Add standard CMake `install` rules utilizing `include(GNUInstallDirs)` to deploy the `organism_sim` standalone binary, the `organism_sim.desktop` launcher, and application icons to their standard system locations.
- **Interface / Contract:**
  ```cmake
  include(GNUInstallDirs)
  install(TARGETS organism_sim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES organism_sim.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES assets/icons/icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/200x200/apps RENAME organism_sim.png)
  install(FILES assets/icons/icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps RENAME organism_sim.png)
  ```
- **Behavior & Edge Cases:**
  - Respects standard `CMAKE_INSTALL_PREFIX` or `--prefix` argument without hardcoded absolute paths.
- **Verification & Acceptance Criteria:**
  - [x] Staging install verifies file placement.
  - [x] Execution verification command:
    ```bash
    cmake --install build --prefix /tmp/organism_sim_test_install
    ```

---

### [TASK-28]: Build Pipeline Verification Suite

- **Prerequisites:** TASK-27.
- **Target Files:**
  - Modify: `tests/test_rules.cpp`
- **Scope & Objective:** Validate build pipeline automation through automated headless assertions confirming installed assets and desktop entry paths exist and are readable.
- **Behavior & Edge Cases:**
  - Confirms zero assertion failures across full test suite.
- **Verification & Acceptance Criteria:**
  - [x] All test cases in `sim_tests` pass 100% cleanly.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/sim_tests
    ```

---

### [TASK-18]: Step Counter State & Interface in Simulation Domain

- **Prerequisites:** TASK-17.
- **Target Files:**
  - Modify: `include/sim/Simulation.hpp`
  - Modify: `src/sim/Simulation.cpp`
- **Scope & Objective:** Add discrete simulation cycle tracking to the core simulation engine. Increment a 64-bit step counter on each invocation of `Simulation::Step`, expose it via a const getter, and reset it back to 0 on `Simulation::Reset()`.
- **Interface / Contract:**
  ```cpp
  class Simulation {
  public:
      [[nodiscard]] uint64_t GetStepCount() const noexcept;
  private:
      uint64_t step_count_{0};
  };
  ```
- **Behavior & Edge Cases:**
  - Counter increments by 1 per `Step(dt)` regardless of `dt` magnitude.
  - Counter resets to 0 when `Reset()` is invoked.
  - Headless `sim_core` maintains zero Raylib dependencies.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly into `sim_core`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_core
    ```

---

### [TASK-19]: TrueType Font Asset Integration & UI Font Management

- **Prerequisites:** TASK-18.
- **Target Files:**
  - Create: `assets/fonts/LiberationSans-Regular.ttf`
  - Modify: `include/app/UI.hpp`
  - Modify: `src/app/UI.cpp`
  - Modify: `src/app/main.cpp`
- **Scope & Objective:** Integrate a clean TrueType font (Liberation Sans) into the project assets. Enhance `UI` to load the font at high resolution ($32\text{px}$) with bilinear texture filtering for crisp downsampling, providing safe fallback to default font and deterministic cleanup on exit.
- **Interface / Contract:**
  ```cpp
  class UI {
  public:
      void Init();
      void Cleanup();
  private:
      Font font_{};
      bool custom_font_loaded_{false};
      void DrawCrispText(const char* text, float x, float y, float size, Color color) const;
  };
  ```
- **Behavior & Edge Cases:**
  - If `assets/fonts/LiberationSans-Regular.ttf` or system fallback is present, load with `LoadFontEx` and `SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR)`.
  - If font file is missing, fallback gracefully to `GetFontDefault()` without crash.
  - On application termination, call `UnloadFont` only if custom font was loaded.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly under `organism_sim`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-20]: Sidebar HUD Step Counter Telemetry & Typography Polishing

- **Prerequisites:** TASK-19.
- **Target Files:**
  - Modify: `src/app/UI.cpp`
- **Scope & Objective:** Display the formatted step counter directly below the Time Scale row in the sidebar HUD (e.g., `Steps: 1,420`). Update all text rendering calls in `UI::DrawSidebar` to use the crisp font rendering with clean spacing and legibility.
- **Interface / Contract:**
  ```cpp
  // Render step counter row directly below time scale row
  draw_text_row("Steps: " + FormatNumber(sim.GetStepCount()), 14, Color{180, 185, 200, 255});
  ```
- **Behavior & Edge Cases:**
  - Step counter updates dynamically every simulation tick.
  - Resetting simulation instantly updates step counter display back to `Steps: 0`.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly into `organism_sim`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-21]: Automated Verification Suite for Step Counter Telemetry

- **Prerequisites:** TASK-20.
- **Target Files:**
  - Modify: `tests/test_rules.cpp`
- **Scope & Objective:** Implement automated headless unit tests verifying `TC-STEP-01` (initial 0), `TC-STEP-02` (step increments), `TC-STEP-03` (reset clear), and `TC-STEP-04` (variable timestep invariant).
- **Behavior & Edge Cases:**
  - Confirms zero assertion failures under AddressSanitizer and UndefinedBehaviorSanitizer.
- **Verification & Acceptance Criteria:**
  - [x] All test cases in `sim_tests` pass 100% cleanly.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/sim_tests
    ```

---

### [TASK-22]: Embedded PNG Icon Header Generation

- **Prerequisites:** TASK-21.
- **Target Files:**
  - Create: `include/app/IconData.hpp`
  - Create: `assets/icons/avatar.jpg`
  - Create: `assets/icons/icon.png`
- **Scope & Objective:** Fetch avatar image from `https://sappsworld.com/images/avatar.jpg`, convert it to optimized PNG, and embed the raw byte array into C++ header `include/app/IconData.hpp` with `ICON_PNG_DATA` and `ICON_PNG_DATA_LEN`.
- **Interface / Contract:**
  ```cpp
  namespace app {
      inline constexpr unsigned char ICON_PNG_DATA[] = { ... };
      inline constexpr unsigned int ICON_PNG_DATA_LEN = sizeof(ICON_PNG_DATA);
  }
  ```
- **Behavior & Edge Cases:**
  - Header-only, constexpr array. No dynamic allocation.
  - Valid PNG magic bytes at `ICON_PNG_DATA[0..7]`.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly under `organism_sim`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-23]: Window Icon Runtime Integration via Raylib

- **Prerequisites:** TASK-22.
- **Target Files:**
  - Modify: `src/app/main.cpp`
- **Scope & Objective:** In `main.cpp` following `InitWindow`, deserialize the embedded PNG bytes using `LoadImageFromMemory(".png", app::ICON_PNG_DATA, app::ICON_PNG_DATA_LEN)`. Apply it using `SetWindowIcon(icon)`, then safely free CPU image memory via `UnloadImage(icon)`.
- **Interface / Contract:**
  ```cpp
  Image icon = LoadImageFromMemory(".png", app::ICON_PNG_DATA, app::ICON_PNG_DATA_LEN);
  if (icon.data != nullptr) {
      SetWindowIcon(icon);
      UnloadImage(icon);
  }
  ```
- **Behavior & Edge Cases:**
  - Safely guards against `icon.data == nullptr` (graceful skip if corrupt).
  - Unloads CPU RAM image after passing texture to display server/GLFW window context.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly under `organism_sim`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-24]: Cross-Platform Executable Branding (Linux .desktop & Windows .rc)

- **Prerequisites:** TASK-23.
- **Target Files:**
  - Create: `organism_sim.desktop`
  - Create: `src/app/resource.rc`
  - Create: `assets/icons/icon.ico`
  - Modify: `CMakeLists.txt`
- **Scope & Objective:** Provide OS-level launcher and executable icons. Generate multi-resolution `icon.ico` (16 to 256 px) and a Windows resource file `resource.rc` conditionally linked for `WIN32` builds. Provide standard FreeDesktop launcher `organism_sim.desktop` for Linux desktop environments.
- **Interface / Contract:**
  ```rc
  IDI_ICON1 ICON "assets/icons/icon.ico"
  ```
  ```desktop
  [Desktop Entry]
  Type=Application
  Name=Organism Sim
  Comment=Autonomous 2D Multi-Tile Organism Simulation (A-Life)
  Exec=organism_sim
  Icon=assets/icons/icon.png
  Terminal=false
  Categories=Game;Simulation;Science;
  ```
- **Behavior & Edge Cases:**
  - `CMakeLists.txt` guards `src/app/resource.rc` with `if(WIN32)` so Linux GCC/Clang builds ignore it without requiring `windres`.
- **Verification & Acceptance Criteria:**
  - [x] Compiles cleanly under `organism_sim`.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target organism_sim
    ```

---

### [TASK-25]: Automated Verification Suite for Icon Data & Launcher Invariants

- **Prerequisites:** TASK-24.
- **Target Files:**
  - Modify: `tests/test_rules.cpp`
- **Scope & Objective:** Implement automated headless unit tests verifying `TC-ICON-01` (PNG magic signature), `TC-ICON-02` (embedded buffer size), `TC-ICON-03` (Windows ICO asset signature), and `TC-ICON-04` (FreeDesktop launcher format).
- **Behavior & Edge Cases:**
  - Confirms zero assertion failures under AddressSanitizer and UndefinedBehaviorSanitizer.
- **Verification & Acceptance Criteria:**
  - [x] All test cases in `sim_tests` pass 100% cleanly.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/sim_tests
    ```

---

### [TASK-26]: Custom Clean and Rebuild Targets in CMakeLists.txt

- **Prerequisites:** TASK-25.
- **Target Files:**
  - Modify: `CMakeLists.txt`
- **Scope & Objective:** Implement custom targets `clean_all` (invoking the clean target on the build directory) and `rebuild` (clean followed by building `organism_sim` and `sim_tests`) to enhance developer workflow usability.
- **Interface / Contract:**
  ```cmake
  add_custom_target(clean_all
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
      COMMENT "Cleaning all project targets..."
  )
  add_custom_target(rebuild
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target clean
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target organism_sim
      COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target sim_tests
      COMMENT "Executing full clean and rebuild..."
  )
  ```
- **Behavior & Edge Cases:**
  - Operates cleanly on both Ninja and Unix Makefiles generators without hardcoded shell calls.
- **Verification & Acceptance Criteria:**
  - [x] `cmake --build build --target clean_all` executes cleanly.
  - [x] `cmake --build build --target rebuild` completes successfully.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target clean_all && cmake --build build --target rebuild
    ```

---

### [TASK-27]: GNUInstallDirs Installation Target for Executable, Desktop Launcher & Icons

- **Prerequisites:** TASK-26.
- **Target Files:**
  - Modify: `CMakeLists.txt`
- **Scope & Objective:** Add standard CMake `install` rules utilizing `include(GNUInstallDirs)` to deploy the `organism_sim` standalone binary, the `organism_sim.desktop` launcher, and application icons to their standard system locations.
- **Interface / Contract:**
  ```cmake
  include(GNUInstallDirs)
  install(TARGETS organism_sim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES organism_sim.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES assets/icons/icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/200x200/apps RENAME organism_sim.png)
  install(FILES assets/icons/icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps RENAME organism_sim.png)
  ```
- **Behavior & Edge Cases:**
  - Respects standard `CMAKE_INSTALL_PREFIX` or `--prefix` argument without hardcoded absolute paths.
- **Verification & Acceptance Criteria:**
  - [x] Staging install verifies file placement.
  - [x] Execution verification command:
    ```bash
    cmake --install build --prefix /tmp/organism_sim_test_install
    ```

---

### [TASK-28]: Build Pipeline Verification Suite

- **Prerequisites:** TASK-27.
- **Target Files:**
  - Modify: `tests/test_rules.cpp`
- **Scope & Objective:** Validate build pipeline automation through automated headless assertions confirming installed assets and desktop entry paths exist and are readable.
- **Behavior & Edge Cases:**
  - Confirms zero assertion failures across full test suite.
- **Verification & Acceptance Criteria:**
  - [x] All test cases in `sim_tests` pass 100% cleanly.
  - [x] Execution verification command:
    ```bash
    cmake --build build --target sim_tests && ./build/sim_tests
    ```



