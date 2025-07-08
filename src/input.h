#pragma once
#include<stdbool.h>

typedef struct {
    int dx, dy;
    bool quit;
} InputState;

void input_poll(InputState *out);