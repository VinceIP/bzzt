#include <stdio.h>
#include "raylib.h"
#include "world.h"
#include "renderer.h"

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);

    World *world = world_create("New World");
    Renderer rend;
    Renderer_Init(&rend, "assets/bzzt_font_8x16.png");
    ClearBackground(BLACK);

    InputState in = {0};

    while (!in.quit)
    {
        input_poll(&in);
        world_update(world, &in);

        BeginDrawing();
        Renderer_DrawBoard(&rend, world->boards[world->boards_current]);
        // renderer_draw();
        EndDrawing();
    }
    world_unload(world);
    Renderer_Quit(&rend);
    CloseWindow();
    return 0;
}