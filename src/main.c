#include "raylib.h"
#include "world.h"
#include "renderer.h"

int main(void)
{
    Renderer rend;
    Renderer_Init(&rend, "assets/bzzt_font_8x16.png");

    World *world = world_create("New World");
    const int screenWidth = world->boards[world->boards_current]->width * rend.glyph_w;
    const int screenHeight = world->boards[world->boards_current]->height * rend.glyph_h;

    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);

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