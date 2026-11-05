# Contributing to Autonomous 2D Organism Simulation

Thank you for your interest in contributing! This project is an artificial life (A-life) multi-agent simulation built with deterministic C++20 and Raylib 5.0.

---

## Code of Conduct & Development Principles

1. **Separation of Concerns:** The core simulation logic (`sim_core` in `include/sim` and `src/sim`) is **completely headless** and decoupled from GUI/graphics libraries. Never include Raylib headers inside `sim_core`.
2. **Determinism & Performance:** Simulation updates run on fixed-timesteps ($0.05\text{s}$). Spatial operations utilize the $100 \times 100$ `GridMatrix` spatial lookup and contiguous memory storage in `EntityManager`.
3. **Memory Safety & Sanitizers:** All contributions must pass the entire test suite with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) enabled with **0 memory leaks and 0 errors**.
4. **Compiler Warnings:** Code must compile cleanly with `-Wall -Wextra -pedantic` under GCC/Clang and `/W4` under MSVC without compiler warnings.

---

## Local Development Workflow

### 1. Prerequisites
- **Compiler:** GCC 11+, Clang 13+, or MSVC 2019/2022 supporting C++20.
- **Build System:** CMake 3.22+.
- **Graphics/System Libraries (Linux):** OpenGL / Mesa, X11 development headers (`libx11-dev`, `libxcursor-dev`, `libxinerama-dev`, `libxrandr-dev`, `libxi-dev`).

### 2. Configure & Build

```bash
# Clone the repository
git clone https://github.com/tpsapp/organism-sim.git
cd organism-sim

# Configure with AddressSanitizer and UndefinedBehaviorSanitizer
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON

# Compile the application and test suite
cmake --build build -j$(nproc)
```

### 3. Running Headless Tests

Run the headless unit and integration test suite before submitting a Pull Request:

```bash
./build/sim_tests
```

All test cases must report `[ OK ]` with zero sanitizer violations.

If you add new simulation rules or behaviors, add corresponding test cases in `tests/test_rules.cpp`, `tests/test_ai_fsm.cpp`, or `tests/test_metabolism.cpp`.

---

## Pull Request Guidelines

1. **Branch Naming:** Use clear branch names like `feature/scent-mutation-refactor` or `fix/grid-boundary-wrap`.
2. **Atomic Commits:** Keep commits focused with conventional commit messages (e.g., `feat: ...`, `fix: ...`, `docs: ...`, `refactor: ...`).
3. **Test Coverage:** Every bug fix must include a regression test; every new simulation feature must include boundary condition unit tests.
