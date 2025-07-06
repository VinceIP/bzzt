#pragma once
#include "world.h"
#include "renderer.h"
#include "input.h"


typedef struct {
    World world;
    Renderer renderer;
    Input input;
    bool running;
} Engine;

bool Engine_Init(Engine*, int width, int heigh);
void Engine_Run(Engine*);
void Engine_Quit(Engine*);