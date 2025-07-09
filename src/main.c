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
    e.state = SPLASH_MODE;
    e.debugShow = false;
    e.font = font;
    //World *world = world_create("New World");
    Renderer rend;

    Renderer_Init(&rend, "assets/bzzt_font_8x16.png");

    InputState in = {0};

    while (!in.quit)
    {
        Input_Poll(&in);
        Engine_Update(&e, &in);
        //World_Update(world, &in);

        BeginDrawing();
        ClearBackground((Color){10, 26, 51, 255});
        Renderer_Update(&rend, &e);
        // Renderer_DrawBoard(&rend, world->boards[world->boards_current]);
        
        //draw_debug(world, &in);
        // renderer_draw();
        EndDrawing();
    }
    //world_unload(world);
    Renderer_Quit(&rend);
    CloseWindow();
    return 0;
}