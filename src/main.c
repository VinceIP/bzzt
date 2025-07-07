#include "raylib.h"
#include "world.h"
#include "renderer.h"

int main(void)
{
    const int screenWidth = 640;
    const int screenHeight = 400;

    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);

    World *world = world_create("New World");
    Renderer rend;
    Renderer_Init(&rend, "assets/bzzt_font_8x16.png");

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        Renderer_DrawBoard(&rend, world->boards[world->boards_current]);
        // input_update();
        // world_update();
        // renderer_draw();
        EndDrawing();
    }
    world_unload(world);
    Renderer_Quit(&rend);
    CloseWindow();
    return 0;
}