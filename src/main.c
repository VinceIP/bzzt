#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"
#include "engine.h"
#include "world.h"
#include "renderer.h"
#include "debugger.h"

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);

    Engine *engine;
    engine->state = SPLASH_MODE;
    engine->debugShow = true;
    World *world = world_create("New World");
    Renderer rend;

    Renderer_Init(&rend, "assets/bzzt_font_8x16.png");
    ClearBackground(BLACK);

    InputState in = {0};

    while (!in.quit)
    {
        Input_Poll(&in);
        World_Update(world, &in);

        BeginDrawing();
        Renderer_Update(&rend, engine);
        Renderer_DrawBoard(&rend, world->boards[world->boards_current]);
        draw_debug(world, &in);
        // renderer_draw();
        EndDrawing();
    }
    world_unload(world);
    Renderer_Quit(&rend);
    CloseWindow();
    return 0;
}