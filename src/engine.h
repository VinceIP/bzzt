#pragma once
#include "world.h"
#include "renderer.h"
#include "input.h"

typedef enum{
    SPLASH_MODE,
    PLAY_MODE,
    EDIT_MODE
} EState;

typedef struct {
    EState state;
    World world;
    // Renderer renderer;
    // Input input;
    bool running;
    bool debugShow;
} Engine;

bool Engine_Init(Engine*, int width, int heigh);
void Engine_Run(Engine*);
void Engine_Quit(Engine*);