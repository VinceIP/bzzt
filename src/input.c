#include <stdbool.h>
#include "raylib.h"
#include "input.h"

void Input_Poll(InputState *s)
{
    //s->elapsedTime = GetTime();
    s->dx = (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT));
    s->dy = (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP));
    s->L_pressed = IsKeyPressed(KEY_L);
    s->Q_pressed = IsKeyPressed(KEY_Q);
    if (s->dx != 0 || s->dy != 0) // If key is down
    {
        if (s->heldFrames == 0)
        {
            s->delayLock = false;
        }
        else if(s->heldFrames >= 4){
            s->delayLock = false;
            s->heldFrames = 0;
        }
        else {
            s->delayLock = true;
        }
        s->heldFrames++;
    }
    else
    {
        s->heldFrames = 0;
        s->delayLock = false;
    }

    s->quit = IsKeyPressed(KEY_ESCAPE) || WindowShouldClose();
}