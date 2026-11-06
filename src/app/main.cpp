#include "sim/Common.hpp"
#include "sim/Simulation.hpp"
#include "app/Renderer.hpp"
#include "app/UI.hpp"
#include "app/IconData.hpp"
#include "raylib.h"

int main() {
    InitWindow(sim::WINDOW_WIDTH_PX, sim::WINDOW_HEIGHT_PX, "Autonomous 2D Multi-Tile Organism Simulation");
    SetTargetFPS(60);

    // Apply embedded application window icon across multiple resolutions (200x200, 64x64, 32x32, 16x16)
    Image icon_base = LoadImageFromMemory(".png", app::ICON_PNG_DATA, app::ICON_PNG_DATA_LEN);
    if (icon_base.data != nullptr) {
        ImageFormat(&icon_base, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Image icon_64 = ImageCopy(icon_base);
        ImageResize(&icon_64, 64, 64);
        Image icon_32 = ImageCopy(icon_base);
        ImageResize(&icon_32, 32, 32);
        Image icon_16 = ImageCopy(icon_base);
        ImageResize(&icon_16, 16, 16);

        Image icons[4] = {icon_base, icon_64, icon_32, icon_16};
        SetWindowIcons(icons, 4);

        UnloadImage(icon_16);
        UnloadImage(icon_32);
        UnloadImage(icon_64);
        UnloadImage(icon_base);
    }

    std::random_device rd;
    sim::Simulation simulation(rd());
    app::Renderer renderer;
    app::UI ui;
    ui.Init();

    sim::ActiveTool active_tool = sim::ActiveTool::FOOD;
    float time_scale = 1.0f;
    bool paused = false;
    std::string notification_msg;
    float notification_timer = 0.0f;

    Camera2D camera{};
    camera.target = Vector2{sim::PLAYFIELD_WIDTH_PX / 2.0f, sim::PLAYFIELD_HEIGHT_PX / 2.0f};
    camera.offset = Vector2{sim::PLAYFIELD_WIDTH_PX / 2.0f, sim::PLAYFIELD_HEIGHT_PX / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    constexpr float FIXED_TIMESTEP = 0.05f; // 20 simulation updates per second
    float accumulator = 0.0f;

    while (!WindowShouldClose()) {
        float frame_dt = GetFrameTime();
        if (frame_dt > 0.25f) frame_dt = 0.25f; // Prevent spiral of death

        // ---------------------------------------------------------------------
        // Input Handling
        // ---------------------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }

        if (IsKeyPressed(KEY_R)) {
            simulation.Reset();
        }

        // Tool Selection
        if (IsKeyPressed(KEY_ONE)) active_tool = sim::ActiveTool::FOOD;
        if (IsKeyPressed(KEY_TWO)) active_tool = sim::ActiveTool::WATER;
        if (IsKeyPressed(KEY_THREE)) active_tool = sim::ActiveTool::MALE;
        if (IsKeyPressed(KEY_FOUR)) active_tool = sim::ActiveTool::FEMALE;

        // Speed Adjustment (+ / -)
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            if (time_scale < 8.0f) time_scale *= 2.0f;
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            if (time_scale > 1.0f) time_scale /= 2.0f;
        }

        // Save / Load System (F5 / F9)
        if (notification_timer > 0.0f) {
            notification_timer -= frame_dt;
            if (notification_timer <= 0.0f) {
                notification_msg.clear();
            }
        }

        if (IsKeyPressed(KEY_F5)) {
            bool ok = simulation.SaveState("saved_simulation.json");
            notification_msg = ok ? "Saved: saved_simulation.json (F5)" : "Save Failed! (F5)";
            notification_timer = 3.5f;
        }

        if (IsKeyPressed(KEY_F9)) {
            bool ok = simulation.LoadState("saved_simulation.json");
            notification_msg = ok ? "Loaded: saved_simulation.json (F9)" : "Load Failed / File Missing (F9)";
            notification_timer = 3.5f;
        }

        // Camera Pan & Zoom Controls
        Vector2 mouse_pos = GetMousePosition();
        bool mouse_in_playfield = (mouse_pos.x >= 0 && mouse_pos.x < sim::PLAYFIELD_WIDTH_PX &&
                                   mouse_pos.y >= 0 && mouse_pos.y < sim::PLAYFIELD_HEIGHT_PX);

        // Zoom with wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && mouse_in_playfield) {
            Vector2 mouse_world_before = GetScreenToWorld2D(mouse_pos, camera);
            camera.offset = mouse_pos;
            camera.target = mouse_world_before;
            camera.zoom = std::clamp(camera.zoom + wheel * 0.15f, 0.75f, 4.0f);
        }

        // Pan with Right Mouse Drag
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && mouse_in_playfield) {
            Vector2 delta = GetMouseDelta();
            delta.x = -delta.x / camera.zoom;
            delta.y = -delta.y / camera.zoom;
            camera.target.x += delta.x;
            camera.target.y += delta.y;
        }

        // Reset Camera ('C' or 'Home')
        if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_HOME)) {
            camera.target = Vector2{sim::PLAYFIELD_WIDTH_PX / 2.0f, sim::PLAYFIELD_HEIGHT_PX / 2.0f};
            camera.offset = Vector2{sim::PLAYFIELD_WIDTH_PX / 2.0f, sim::PLAYFIELD_HEIGHT_PX / 2.0f};
            camera.zoom = 1.0f;
        }

        // ---------------------------------------------------------------------
        // Mouse Hover & Placement
        // ---------------------------------------------------------------------
        const sim::Organism* hovered_org = nullptr;
        int hover_cell_x = -1;
        int hover_cell_y = -1;

        if (mouse_in_playfield) {
            Vector2 world_pos = GetScreenToWorld2D(mouse_pos, camera);
            hover_cell_x = static_cast<int>(world_pos.x) / sim::CELL_SIZE_PX;
            hover_cell_y = static_cast<int>(world_pos.y) / sim::CELL_SIZE_PX;
            hovered_org = simulation.GetOrganismAt(hover_cell_x, hover_cell_y);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse_in_playfield) {
            switch (active_tool) {
                case sim::ActiveTool::FOOD:
                    simulation.ManualPlace(hover_cell_x, hover_cell_y, sim::EntityType::FOOD);
                    break;
                case sim::ActiveTool::WATER:
                    simulation.ManualPlace(hover_cell_x, hover_cell_y, sim::EntityType::WATER);
                    break;
                case sim::ActiveTool::MALE:
                    simulation.ManualPlace(hover_cell_x, hover_cell_y, sim::EntityType::ORGANISM, sim::Gender::MALE);
                    break;
                case sim::ActiveTool::FEMALE:
                    simulation.ManualPlace(hover_cell_x, hover_cell_y, sim::EntityType::ORGANISM, sim::Gender::FEMALE);
                    break;
            }
        }

        // ---------------------------------------------------------------------
        // Simulation Tick
        // ---------------------------------------------------------------------
        if (!paused) {
            accumulator += frame_dt * time_scale;
            while (accumulator >= FIXED_TIMESTEP) {
                simulation.Step(FIXED_TIMESTEP);
                accumulator -= FIXED_TIMESTEP;
            }
        }

        // ---------------------------------------------------------------------
        // Rendering
        // ---------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(BLACK);

        BeginScissorMode(0, 0, sim::PLAYFIELD_WIDTH_PX, sim::PLAYFIELD_HEIGHT_PX);
        BeginMode2D(camera);

        renderer.DrawPlayfield(simulation, hovered_org);

        EndMode2D();
        EndScissorMode();

        ui.DrawSidebar(simulation, active_tool, time_scale, paused, hovered_org, notification_msg);

        EndDrawing();
    }

    ui.Cleanup();
    CloseWindow();
    return 0;
}

