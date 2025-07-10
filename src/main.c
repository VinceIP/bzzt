#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"
#include "engine.h"
#include "input.h"
#include "world.h"
#include "renderer.h"
#include "debugger.h"

static void setup_raylib()
{
    const int screenWidth = 1280;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);
}

int main(void)
{
    setup_raylib();
    Font font = LoadFont("assets/Perfect DOS VGA 437 Win.ttf");
    Engine e;
    Engine_Init(&e);
    e.font = font;
    e.world = world_create("New World");
    Renderer rend;

    Renderer_Init(&rend, "assets/bzzt_font_8x16.png");

    InputState in = {0};

    while (!in.quit)
    {
        Engine_Update(&e, &in);
        BeginDrawing();
        Renderer_Update(&rend, &e);
        EndDrawing();
    }
    Engine_Quit(&e);
    Renderer_Quit(&rend);
    CloseWindow();
    return 0;
}