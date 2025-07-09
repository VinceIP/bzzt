#pragma once
#include<stdbool.h>

struct Engine;

typedef struct {
    int dx, dy;
    bool L_pressed;
    bool Q_pressed;
    bool quit;
    int heldFrames;
    double elapsedTime;
    const int frameDelay;
    bool delayLock;
} InputState;

void Input_Poll(InputState *out);