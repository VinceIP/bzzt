#pragma once
#include <stdbool.h>
#include "raylib.h"

typedef struct BzztCamera BzztCamera;

typedef struct InputState
{
    int dx, dy; // Target input direction
    bool anyDirPressed;
    bool E_pressed;
    bool L_pressed;
    bool Q_pressed;
    bool quit;
    int heldFrames;
    double elapsedTime;
    const int frameDelay;
    bool delayLock;
} InputState;

typedef struct MouseState
{
    Vector2 screenPosition, worldPosition, delta;
    Vector2 lastScreenPosition, lastWorldPosition;
    bool moved;
    bool leftPressed, rightPressed, middlePressed, leftDown, rightDown, middleDown;
    float wheelMove;
} MouseState;

void Input_Poll(InputState *out);
void Mouse_Poll(MouseState *out);
void Mouse_Reset(MouseState *out);
Vector2 Handle_Cursor_Move(InputState *in, MouseState *m, BzztCamera *c, Rectangle bounds);