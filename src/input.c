#include "raylib.h"
#include "input.h"

void input_poll(InputState *s)
{
    s->dx = (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT));
    s->dy = (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP));
    s->quit = IsKeyPressed(KEY_ESCAPE) || WindowShouldClose();
}