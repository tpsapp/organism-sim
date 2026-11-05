# Bug Fix Report: Simulation Step Counter Alignment with Organism 1.0s Movement Cadence

- **Issue ID:** FIX-05
- **File Target:** `.planning/fixes/step-counter-time-alignment.md`
- **Component:** Core Simulation Engine (`Simulation.hpp`, `Simulation.cpp`, `test_rules.cpp`)
- **Status:** **VERIFIED & RESOLVED**

---

### [BUG-FIX]: Step Counter Rate Mismatch with Organism Movement Cadence

- **Defect Summary:** The step counter was incrementing 20 times too fast relative to organism actions. Organisms move at a cadence of 1 step per simulated second (`BASE_STEP_INTERVAL = 1.0f`), but the step counter incremented on every discrete sub-timestep integration tick (`FIXED_TIMESTEP = 0.05f`), resulting in 20 counter increments per real second.
- **Root Cause Analysis:**
  1. In `src/app/main.cpp`, the game loop performs fixed-interval physics sub-stepping with `FIXED_TIMESTEP = 0.05f` (20 updates/second) to ensure smooth interpolation and collision resolution.
  2. In `src/sim/Simulation.cpp`, `Step(float dt)` unconditionally ran `++step_count_;` regardless of the size of `dt`.
  3. Consequently, for every 1.0 second of simulated time (in which organisms take exactly 1 movement step), `Step(0.05f)` ran 20 times and advanced `step_count_` by 20.
- **Remediation:**
  1. Introduced `float step_accumulator_{0.0f};` to `Simulation`.
  2. In `Simulation::Step(float dt)`, accumulate elapsed simulation delta time:
     ```cpp
     step_accumulator_ += dt;
     while (step_accumulator_ >= (BASE_STEP_INTERVAL - 1e-5f)) {
         step_accumulator_ = std::max(0.0f, step_accumulator_ - BASE_STEP_INTERVAL);
         ++step_count_;
     }
     ```
  3. Reset `step_accumulator_ = 0.0f;` and `step_count_ = 0;` upon `Simulation::Reset()`.
  4. The counter now advances by exactly 1 step per 1.0 second of simulation time, remaining perfectly synchronized with the organisms' base movement cooldown and biological clock.

---

### Regression Test Harness

```cpp
TEST_CASE(TC_STEP_04_Organism_One_Second_Step_Alignment) {
    Simulation sim(123);
    // 19 sub-ticks of 0.05s = 0.95s elapsed -> 0 full organism steps (requires 1.0s)
    for (int i = 0; i < 19; ++i) {
        sim.Step(0.05f);
    }
    CHECK_EQ(sim.GetStepCount(), 0ULL);

    // 20th sub-tick reaches 1.00s elapsed -> exactly 1 organism step
    sim.Step(0.05f);
    CHECK_EQ(sim.GetStepCount(), 1ULL);

    // Another 20 sub-ticks reaches 2.00s elapsed -> exactly 2 organism steps
    for (int i = 0; i < 20; ++i) {
        sim.Step(0.05f);
    }
    CHECK_EQ(sim.GetStepCount(), 2ULL);
}
```

---

### Verification Commands & Evidence

1. **Pre-patch Test Execution:**
   - Command: `cmake --build build --target sim_tests && ./build/sim_tests`
   - Output:
     ```text
     [ RUN      ] TC_STEP_04_Organism_One_Second_Step_Alignment
       FAILED: (sim.GetStepCount() == 0ULL) [19 != 0]
       FAILED: (sim.GetStepCount() == 1ULL) [20 != 1]
       FAILED: (sim.GetStepCount() == 2ULL) [40 != 2]
     [  FAILED  ] TC_STEP_04_Organism_One_Second_Step_Alignment
     ```
2. **Post-patch Test Execution:**
   - Command: `cmake --build build --target sim_tests && ./build/sim_tests`
   - Output:
     ```text
     [ RUN      ] TC_STEP_04_Organism_One_Second_Step_Alignment
     [       OK ] TC_STEP_04_Organism_One_Second_Step_Alignment
     =======================================
      Test Summary: 59 passed, 0 failed (296 total assertions)
     =======================================
     ```
3. **Application Build:**
   - Executable `./build/organism_sim` built with zero warnings.

---

### Side-Effect Audit
- [x] Public API contracts preserved.
- [x] Simulation physics sub-stepping at 0.05s preserved.
- [x] Organism movement rates and cooldown calculations untouched.
- [x] All 59 test cases pass under AddressSanitizer and UndefinedBehaviorSanitizer.

---

*Lifecycle Citation:*
Following the Engineering Lifecycle Workflow:
- To initialize version control, check hygiene, and publish the repository to GitHub, invoke **`github-repo-publisher`**.

