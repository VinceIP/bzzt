#pragma once

#define VIEWPORT_DEFAULT_W = 60
#define VIEWPORT_DEFAULT_H = 25

struct Rectangle;
struct Vector2;

typedef struct Viewport
{
    int x, y;
    Rectangle rect;
    Vector2 origin;
} Viewport;

typedef struct BzztCamera
{
    Viewport viewport;
} BzztCamera;