# Autonomous 2D Multi-Tile Organism Simulation (A-Life)

[![C++20](https://img.shields.io/badge/standard-C%2B%2B20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/build-CMake_3.22+-064F8C.svg?logo=cmake)](https://cmake.org/)
[![Raylib](https://img.shields.io/badge/graphics-Raylib_5.0-black.svg?logo=raylib)](https://www.raylib.com/)
[![Sanitizers](https://img.shields.io/badge/tests-ASan_%26_UBSan_Passing-success.svg)](#running-the-test-suite)
[![Platform](https://img.shields.io/badge/platform-Linux_%7C_Windows-lightgrey.svg)](#compilation--build-guide)

A deterministic, multi-agent artificial life (A-life) simulation built in **C++20** and **Raylib 5.0**. Organisms with dynamic multi-tile footprints ($1 \times 1$ up to $4 \times 4$) inhabit a discrete $100 \times 100$ toroidal grid environment, exhibiting emergent lifecycles, dual-mode metabolic regeneration, Chebyshev sensory scanning, scent diffusion, Mendelian-like inheritance, and priority-driven finite state machines (FSM).

---

## Table of Contents

- [Features](#features)
- [Simulation Mechanics](#simulation-mechanics)
  - [Metabolism & Dual Healing](#metabolism--dual-healing)
  - [Reproduction & Genetics](#reproduction--genetics)
  - [Sensory & Decision Engine](#sensory--decision-engine)
  - [Dynamic Multi-Tile Growth](#dynamic-multi-tile-growth)
- [Controls & User Guide](#controls--user-guide)
  - [Interactive Sidebar HUD](#interactive-sidebar-hud)
  - [Keyboard & Mouse Controls](#keyboard--mouse-controls)
- [Project Architecture](#project-architecture)
- [Compilation & Build Guide](#compilation--build-guide)
  - [Prerequisites by Platform](#prerequisites-by-platform)
  - [Building on Linux (Arch, Ubuntu/Debian, Fedora, openSUSE)](#building-on-linux)
  - [Building on Windows (Visual Studio / MSVC)](#building-on-windows)
  - [Custom Build Targets (`clean_all`, `rebuild`)](#custom-build-targets)
  - [Installation Target (`install`)](#installation-target)
- [Running the Test Suite](#running-the-test-suite)
- [License & Assets](#license--assets)

---

## Features

- **Multi-Tile Footprint Collision Engine:** Organisms occupy rectangular footprints ranging from $1 \times 1$ up to $4 \times 4$ cells with strict bounding-box collision handling.
- **Pure Decoupled Architecture (`sim_core`):** Core mathematical and biological rules are compiled into a pure, headless static library completely isolated from GUI and window manager dependencies.
- **Real-Time Sidebar Inspector:** Hover over any organism on the playfield to inspect detailed real-time telemetry (ID, gender, dimensions, life force, satiety, age, damage, scent, FSM state, and gestation countdown).
- **Embedded Zero-Dependency Assets:** TrueType vector typography (Liberation Sans) and application icons are embedded directly into C++ header byte arrays, guaranteeing reliable runtime rendering without filesystem dependencies.
- **Cross-Platform Launcher Branding:** Features embedded Windows PE resource icons (`.ico` / `.rc`) and FreeDesktop compliant launcher integration (`.desktop`).
- **Memory-Safe Verification:** 100% test pass rate across 61 automated unit and integration tests under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

---

## Simulation Mechanics

### Metabolism & Dual Healing
Organisms possess a continuous biological metabolism:
- **Thirst Depletion:** Decreases by $0.10/\text{s}$.
- **Hunger Depletion:** Decreases by $0.05/\text{s}$ (increases to $0.075/\text{s}$ when exhausted at 0 calories).
- **Movement Cost:** Moving consumes $1$ calorie per cell step.
- **Starvation & Dehydration Damage:** If hunger or thirst reaches $0.0$, life force decays at $2.0/\text{s}$.
- **Dual Healing System:**
  1. **Passive Regeneration:** When both hunger and thirst remain above 50% (`hunger > 5.0` and `thirst > 5.0`), life force steadily regenerates at $+1.0/\text{s}$ up to the $100.0$ ceiling.
  2. **Nutritional Healing Burst:** Eating food restores hunger to $10.0$, grants $+40$ calories, and immediately restores a randomized burst of $[5.0, 15.0]$ life force.

### Reproduction & Genetics
- **Maturity & Readiness:** Organisms must reach maturity (`age >= 2`) and maintain sufficient satiety (`hunger > 6.0`, `thirst > 6.0`) to become eligible for mating.
- **Scent Compatibility Check:** Scent tags are 16-bit identifiers. Mating requires compatibility:
  $$\Delta \text{scent} \bmod 10 < 7$$
- **Gestation & Offspring Birth:** Females gestate for $90$ seconds. Upon completion, a $1 \times 1$ juvenile is born into an adjacent unoccupied cell. Offspring inherit averaged parental traits (sight range, smell range, scent signature) with randomized mutation jitter.

### Sensory & Decision Engine
Organisms execute a priority-based Finite State Machine (FSM):
1. **Critical Need (`CRITICAL_NEED`):** If hunger or thirst drops $\le 3.0$, organisms prioritize pathfinding toward the closest reachable sustenance.
2. **Reproduction (`SEEK_MATE`):** If mature and well-fed, organisms seek compatible mates within olfactory range.
3. **Exploration (`WANDER`):** Organisms explore unoccupied adjacent cells with 8-directional obstacle avoidance.

### Dynamic Multi-Tile Growth
As organisms consume food and survive across seasons, their lifetime calorie count triggers morphological expansion:
- **$1 \times 1 \rightarrow 2 \times 1$ ($2$ cells):** Reached at $80$ lifetime calories.
- **$2 \times 1 \rightarrow 2 \times 2$ ($4$ cells):** Reached at $200$ lifetime calories.
- **$2 \times 2 \rightarrow 3 \times 3$ ($9$ cells):** Reached at $450$ lifetime calories.
- **$3 \times 3 \rightarrow 4 \times 4$ ($16$ cells):** Maximum size ceiling.

---

## Controls & User Guide

```
┌────────────────────────────────────────────────────────┬──────────────────────┐
│                                                        │  ORGANISM SIM        │
│                                                        │  A-Life Engine       │
│                                                        │  ──────────────────  │
│                                                        │  STATUS: RUNNING     │
│                 100 x 100 PLAYFIELD                    │  Time Scale: 1x      │
│                 (1000 x 1000 px)                       │  Steps: 1,420        │
│                                                        │  ──────────────────  │
│  - Food: Burgundy Red tiles                            │  GRID OCCUPANCY      │
│  - Water: Slate Gray tiles                             │  1,250 / 10,000 (12%)│
│  - Males: Vibrant Blue (health-shaded)                 │  [████             ] │
│  - Females: Vibrant Yellow (health-shaded)             │  ──────────────────  │
│  - Hover: Dual Gold/White highlight box                │  POPULATION STATS    │
│                                                        │  Total: 84           │
│                                                        │  Males: 42           │
│                                                        │  Females: 42         │
│                                                        │  ──────────────────  │
│                                                        │  MANUAL PLACEMENT    │
│                                                        │  [■] [1] Food        │
│                                                        │  [■] [2] Water       │
│                                                        │  [■] [3] Male (10/10)│
│                                                        │  [■] [4] Female (10) │
│                                                        │  ──────────────────  │
│                                                        │  ORGANISM INSPECTOR  │
│                                                        │  ID #14 (Female)     │
│                                                        │  Size: 2x2 | Dmg: 1  │
│                                                        │  Life: 94.2/100      │
└────────────────────────────────────────────────────────┴──────────────────────┘
```

### Keyboard & Mouse Controls

| Input | Action | Description |
| :--- | :--- | :--- |
| `Space` | **Pause / Resume** | Halts or resumes simulation time stepping. |
| `+` / `=` / `KP_+` | **Speed Up** | Increases simulation time scale ($1\times \rightarrow 2\times \rightarrow 4\times \rightarrow 8\times$). |
| `-` / `KP_-` | **Slow Down** | Decreases simulation time scale ($8\times \rightarrow 4\times \rightarrow 2\times \rightarrow 1\times$). |
| `1` | **Select Food Tool** | Unlimited manual placement of Food items ($40$ calories). |
| `2` | **Select Water Tool** | Unlimited manual placement of Water items. |
| `3` | **Select Male Tool** | Manual placement of male organisms (quota: up to 10 per session). |
| `4` | **Select Female Tool** | Manual placement of female organisms (quota: up to 10 per session). |
| `Left Click` | **Place Entity** | Places selected tool entity at the mouse cursor position on the grid. |
| `Mouse Hover` | **Inspect Organism** | Highlights organism footprint and populates the bottom inspector card. |
| `R` | **Reset Simulation** | Clears grid, resets quotas and step counters, and re-seeds world. |

---

## Project Architecture

```
organism-sim/
├── CMakeLists.txt                 # Build configuration, targets, and installation rules
├── organism_sim.desktop           # FreeDesktop Linux application launcher
├── include/
│   ├── sim/                       # Headless Core Simulation Engine
│   │   ├── Common.hpp             # Grid constants, enums, color definitions, vector math
│   │   ├── Entity.hpp             # Resource & Organism entity structures
│   │   ├── EntityManager.hpp      # Memory-contiguous entity storage & lookup
│   │   ├── GridMatrix.hpp         # 100x100 spatial grid & multi-tile collision detection
│   │   ├── AIEngine.hpp           # Sensory scanning, pathfinding & FSM transitions
│   │   └── Simulation.hpp         # Master simulation loop, timers, step counter telemetry
│   └── app/                       # Presentation & Raylib Graphical Application
│       ├── IconData.hpp           # Embedded 32-bit RGBA window icon byte stream
│       ├── Renderer.hpp           # Playfield tile, gridline & entity drawing
│       └── UI.hpp                 # Sidebar HUD, vector font management & inspector
├── src/
│   ├── sim/                       # Simulation Engine Implementations (sim_core)
│   │   ├── GridMatrix.cpp
│   │   ├── EntityManager.cpp
│   │   ├── AIEngine.cpp
│   │   └── Simulation.cpp
│   └── app/                       # Application Implementations
│       ├── Renderer.cpp
│       ├── UI.cpp
│       ├── resource.rc            # Windows PE executable icon script
│       └── main.cpp               # Game loop, input handling, fixed-timestep accumulator
├── assets/
│   ├── fonts/                     # Bundled TrueType vector fonts (Liberation Sans)
│   └── icons/                     # Multi-resolution icons (.png, .ico, .jpg)
├── tests/                         # Headless C++20 Test Harness & Matrix
│   ├── TestHarness.hpp
│   ├── test_main.cpp
│   ├── test_grid.cpp
│   ├── test_metabolism.cpp
│   ├── test_ai_fsm.cpp
│   └── test_rules.cpp
└── .planning/                     # Lifecycle engineering & audit artifacts
```

---

## Compilation & Build Guide

### Prerequisites by Platform

#### Arch Linux / Manjaro
```bash
sudo pacman -S --needed base-devel cmake git libglvnd libx11 libxcursor libxinerama libxrandr libxi
```

#### Ubuntu / Debian / Linux Mint
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev
```

#### Fedora / RHEL / CentOS Stream
```bash
sudo dnf install -y gcc-c++ cmake git mesa-libGL-devel libX11-devel libXcursor-devel libXinerama-devel libXrandr-devel libXi-devel
```

#### openSUSE (Tumbleweed / Leap)
```bash
sudo zypper install -y gcc-c++ cmake git Mesa-libGL-devel libX11-devel libXcursor-devel libXinerama-devel libXrandr-devel libXi-devel
```

#### Windows
- **Visual Studio 2019 / 2022** with the **"Desktop development with C++"** workload installed.
- **CMake 3.22+** (included with Visual Studio or available via `winget install Kitware.CMake`).
- **Git** (available via `winget install Git.Git`).

---

### Building on Linux

```bash
# 1. Clone repository
git clone https://github.com/your-username/organism-sim.git
cd organism-sim

# 2. Configure build tree (Release optimization)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile the interactive application
cmake --build build --target organism_sim -j$(nproc)

# 4. Launch simulation
./build/organism_sim
```

---

### Building on Windows

#### Option A: Visual Studio GUI
1. Open Visual Studio.
2. Select **File > Open > Folder...** and choose the `organism-sim` directory.
3. Visual Studio will automatically detect `CMakeLists.txt` and configure the project.
4. Select `organism_sim.exe` from the target dropdown and click **Run** (or press `F5`).

#### Option B: Developer Command Prompt / PowerShell
```powershell
# 1. Configure for MSVC 64-bit
cmake -B build -G "Visual Studio 17 2022" -A x64

# 2. Compile Release binary
cmake --build build --config Release --target organism_sim

# 3. Launch application
.\build\Release\organism_sim.exe
```

---

### Custom Build Targets

The project includes custom build workflow targets:

- **`clean_all`:** Completely purges all compiled objects and target artifacts:
  ```bash
  cmake --build build --target clean_all
  ```
- **`rebuild`:** Executes a full clean followed by a clean rebuild of both the application and the test runner:
  ```bash
  cmake --build build --target rebuild
  ```

---

### Installation Target

Install the standalone executable, desktop launcher, and application icons to system paths using standard `GNUInstallDirs`:

```bash
# Default install to /usr/local
sudo cmake --install build

# Or install to a user-specified prefix:
cmake --install build --prefix ~/.local
```

Installed files layout:
- Binary: `<prefix>/bin/organism_sim`
- FreeDesktop Launcher: `<prefix>/share/applications/organism_sim.desktop`
- Icon (Hicolor): `<prefix>/share/icons/hicolor/200x200/apps/organism_sim.png`
- Icon (Pixmaps): `<prefix>/share/pixmaps/organism_sim.png`

---

## Running the Test Suite

The test suite runs **100% headless** without requiring an active graphical environment or display server.

```bash
# 1. Configure with AddressSanitizer and UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON

# 2. Build the headless test runner
cmake --build build --target sim_tests

# 3. Execute the full test suite (61 test cases, 309 assertions)
./build/sim_tests
```

Sample output:
```text
=======================================
 Running Headless Simulation Test Suite
=======================================
[ RUN      ] TC_GRID_01_HappyPath_1x1
[       OK ] TC_GRID_01_HappyPath_1x1
...
[ RUN      ] TC_BUILD_01_CMakeLists_Targets_Configured
[       OK ] TC_BUILD_01_CMakeLists_Targets_Configured
=======================================
 Test Summary: 61 passed, 0 failed (309 total assertions)
=======================================
```

---

## License & Assets

- **Source Code:** Distributed under the MIT License.
- **Font:** [Liberation Sans](https://github.com/liberationfonts/liberation-fonts) distributed under the SIL Open Font License (OFL-1.1).
- **Graphics Library:** [Raylib](https://github.com/raysan5/raylib) distributed under the zlib/libpng license.

