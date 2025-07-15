#pragma once
#include <stdbool.h>

typedef struct InputState
{
    Vector2 mouse_screen; // Coords in screen pixels
    Vector2 mouse_world;  // Coords in world units (glyphs on screen)

    int dx, dy; // Target input direction
    bool E_pressed;
    bool L_pressed;
    bool Q_pressed;
    bool quit;
    int heldFrames;
    double elapsedTime;
    const int frameDelay;
    bool delayLock;
} InputState;

void Input_Poll(InputState *out);
void Handle_Key_Move(int *dx, int *dy, Rectangle bounds, InputState *in);