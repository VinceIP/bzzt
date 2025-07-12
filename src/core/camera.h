#pragma once
#include "raylib.h"

#define VIEWPORT_DEFAULT_W = 60
#define VIEWPORT_DEFAULT_H = 25

typedef struct
{
    int x, y;
    Rectangle rect;
    Vector2 origin;
} Viewport;

typedef struct
{
    Viewport viewport;
} BzztCamera;