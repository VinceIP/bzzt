#pragma once
#include "camera.h"
#include "raylib.h"

#define VIEWPORT_DEFAULT_W 60
#define VIEWPORT_DEFAULT_H 25

typedef struct Viewport
{
    Rectangle rect; //x, y, w, h in world units
} Viewport;

typedef struct BzztCamera
{
    Rectangle rect; //x,y,w,h in world units
    int cell_width, cell_height; //Dimensions of cells in pixels
    Viewport viewport; //viewport displaying this camera
} BzztCamera;

BzztCamera* BzztCamera_Create();

