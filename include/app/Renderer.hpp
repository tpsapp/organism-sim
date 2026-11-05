#pragma once

#include "sim/Simulation.hpp"
#include "raylib.h"

namespace app {

class Renderer {
public:
    Renderer() = default;

    void DrawPlayfield(const sim::Simulation& sim, const sim::Organism* hovered_org = nullptr);
    void DrawGridLines();
    void DrawResources(const sim::Simulation& sim);
    void DrawOrganisms(const sim::Simulation& sim, const sim::Organism* hovered_org = nullptr);

private:
    static Color GetOrganismColor(sim::Gender gender, float life_force);
};

} // namespace app

