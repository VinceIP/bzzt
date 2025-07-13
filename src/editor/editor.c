#include <stdbool.h>
#include <stdio.h>
#include "raylib.h"
#include "editor.h"
#include "engine.h"
#include "input.h"
#include "cell.h"
#include "ui.h"
#include "ui_layer.h"
#include "ui_surface.h"
#include "ui_overlay.h"
#include "ui_element.h"
#include "camera.h"

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
    // Programmatically make a sidebar - not efficent or permananet but cool
    UI *ui = e->ui;

    // Create a gray bg
    int sidebar_w = 20;
    int sidebar_h = 25;
    UISurface *sidebar = UISurface_Create(sidebar_w * sidebar_h);
    sidebar->w = sidebar_w;
    sidebar->h = sidebar_h;
    sidebar->x = 60;
    sidebar->y = 0;

    for (int i = 0; i < sidebar_w * sidebar_h; ++i)
    {
        sidebar->cells[i].bg = COLOR_CYAN;
        sidebar->cells[i].glyph = 255;
        sidebar->cells[i].fg = COLOR_WHITE;
        sidebar->cells[i].visible = true;
    }

    UI_Add_Surface(ui, sidebar);

    // Show text on it
    // UIOverlay *o = UIOverlay_Create();
    // UIText *t = UIText_Create(0, 0, COLOR_WHITE, COLOR_DARK_GRAY, "Sample text", NULL);
    // UIOverlay_Add_Element(o, &t->base);
    // UISurface_Add_Overlay(sidebar, o);
    int x = sidebar->x;
    int y = sidebar->y;
    int testX = sidebar->x;
    int testY = sidebar->y;

    UI_Print_Screen(ui, COLOR_WHITE, COLOR_DARK_GRAY, false, x, y, "Sidebar X: %d\nSidebar Y: %d", testX, testY);
    UI_Print_Screen(ui, COLOR_WHITE, COLOR_DARK_GRAY, false, x + 10, y + 10, "Hello world world world world world world world world world world world world world", testX, testY);
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
