#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "raylib.h"

typedef struct Bzzt_Camera Bzzt_Camera;

typedef void (*Key_Handler)(int key);

typedef struct InputState
{
    int dx, dy; // Target input direction
    bool anyDirPressed;
    bool E_pressed;
    bool L_pressed;
    bool Q_pressed;
    bool P_pressed;
    bool quit;
    int heldFrames;
    double elapsedTime;
    const int frameDelay;
    bool delayLock;
    Key_Handler key_handler;
} InputState;

typedef struct MouseState
{
    Vector2 screenPosition, worldPosition, delta;
    Vector2 lastScreenPosition, lastWorldPosition;
    bool moved;
    bool leftPressed, rightPressed, middlePressed, leftDown, rightDown, middleDown;
    float wheelMove;
} MouseState;

void Input_Set_Handler(InputState *in, Key_Handler h);
void Input_Poll(InputState *out);
void Mouse_Poll(MouseState *out);
Vector2 Handle_Cursor_Move(Vector2 currentPos, InputState *in, MouseState *m, Bzzt_Camera *c, Rectangle bounds);