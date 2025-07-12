#include <stdbool.h>
#include "raylib.h"
#include "editor.h"
#include "engine.h"
#include "input.h"
#include "ui.h"

static void handle_cursor(Engine *e, InputState *in)
{
    Cursor *c = &e->cursor;
    int *dx = &c->x;
    int *dy = &c->y;
    Rectangle bounds = e->camera->viewport.rect; // Bounds should be viewport Rect
    Handle_Key_Move(dx, dy, bounds, in);
}

static void ui_init(Engine *e)
{
    char *path = "assets/ui/ui.psci";
    UISurface *surface = UISurface_Load_From_Playscii(path);
    UI *ui = e->ui;
    UI_Add_Surface(ui, surface);
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

    e->camera->viewport.rect = (Rectangle){0, 0, 60, 25};

    ui_init(e);
}

void Editor_Update(Engine *e, InputState *in)
{
    handle_cursor(e, in);
}
