#pragma once
#include <stdbool.h>

typedef struct InputState
{
    int dx, dy; // Target input direction
    int msx, msy;// Mouse coords in screen pixels
    int mcx, mcy; //Mouse coords in cell coordinates
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