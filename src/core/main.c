#include <stdio.h>
#include <stdbool.h>
#include "raylib.h"
#include "engine.h"
#include "input.h"
#include "bzzt.h"
#include "renderer.h"
#include "debugger.h"

#define ASSET(rel) "assets/" rel

static void setup_raylib()
{
    SetTraceLogLevel(LOG_NONE);
    const int screenWidth = 1280;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Bzzt - prototype");
    SetTargetFPS(60);
}

int main(void)
{
    setup_raylib();
    const char *fontPath = ASSET("fonts/default.bzc");
    Engine e;
    bool ok = Engine_Init(&e);
    if (!ok)
    {
        Debug_Printf(LOG_ENGINE, "Engine init failure.");
        return -1;
    }
    else
        Debug_Printf(LOG_ENGINE, "Finished engine init.");
    e.font = GetFontDefault();
    e.world = Bzzt_World_Create("New World");
    Renderer rend;

    Renderer_Init(&rend, &e, fontPath);

    InputState in = {0};
    MouseState mo = {0};

    while (!in.quit)
    {
        Engine_Update(&e, &in, &mo);
        BeginDrawing();
        Renderer_Update(&rend, &e);
        EndDrawing();
    }
    Engine_Quit(&e);
    Renderer_Quit(&rend);
    CloseWindow();
    return 0;
}