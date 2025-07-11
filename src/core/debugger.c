#include "raylib.h"
#include "world.h"
#include "input.h"

void draw_debug(InputState *s)
{
    DrawText(TextFormat("FPS:%2d", GetFPS()), 8, 8, 20, RAYWHITE);
    DrawText(TextFormat("Elapsed time:%.2f", s->elapsedTime), 8, 28, 20, RAYWHITE);
        DrawText(TextFormat("Delay lock:%d", s->delayLock), 8, 48, 20, RAYWHITE);
                DrawText(TextFormat("Held frames:%d", s->heldFrames), 8, 68, 20, RAYWHITE);



}