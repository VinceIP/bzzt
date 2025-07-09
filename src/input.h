#pragma once
#include<stdbool.h>

typedef struct {
    int dx, dy;
    bool quit;
    int heldFrames;
    double elapsedTime;
    const int frameDelay;
    bool delayLock;
} InputState;

void Input_Poll(InputState *out);