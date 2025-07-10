#include <stdbool.h>
#include "raylib.h"
#include "editor.h"
#include "engine.h"
#include "input.h"

static void handle_cursor(Engine *e, InputState *in)
{
    Cursor *c = &e->cursor;
    int *dx = &c->x;
    int *dy = &c->y;
    Rectangle bounds = {0, 0, 85, 25}; // Bounds should be viewport Rect
    Handle_Key_Move(dx, dy, bounds, in);
}

void Editor_Update(Engine *e, InputState *in)
{
    handle_cursor(e, in);
}

void Editor_Init(Engine *e)
{
    e->edit_mode_init_done = true;
    Cursor *c = &e->cursor;
    c->color = COLOR_WHITE;
    c->blinkRate = 0.5;
    c->glyph = 219;
    c->x = 41;
    c->y = 14;
    c->visible = true;
    c->enabled = true;
    c->lastBlink = GetTime();
}