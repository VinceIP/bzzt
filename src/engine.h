#pragma once
#include "raylib.h"
#include "world.h"

struct InputState;

typedef enum
{
    SPLASH_MODE,
    PLAY_MODE,
    EDIT_MODE
} EState;

typedef struct
{
    EState state;
    World world;
    Font font;
    // Renderer renderer;
    // Input input;
    bool running;
    bool debugShow;
} Engine;

bool Engine_Init(Engine *);
void Engine_Update(Engine *, InputState *);
void Engine_Quit(Engine *);