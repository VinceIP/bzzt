#include "raylib.h"
#include "world.h"


int main(void)
{
    const int screenWidth = 640;
    const int screenHeight = 400;

    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);

    World *world = world_create("New World");

    while (!WindowShouldClose())
    {
        //input_update();
        //world_update();
        //renderer_draw();
    }
    world_unload(world);
    CloseWindow();
    return 0;
}