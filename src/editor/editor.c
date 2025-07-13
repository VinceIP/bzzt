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
#include "ui_bindings.h"
#include "text.h"
#include "camera.h"

int testX = 0;

static void handle_cursor(Engine *e, InputState *in)
{
    Cursor *c = &e->cursor;
    int *dx = &c->x;
    int *dy = &c->y;
    Rectangle bounds = e->camera->viewport.rect; // Bounds should be viewport Rect
    Handle_Key_Move(dx, dy, bounds, in);
}

static const char *caption_cb(void *ud) { return (const char *)ud; }

static void ui_init(Engine *e)
{
    int cx = e->cursor.x;
    // int cy = e->cursor.y;
    UISurface *sidebarSurface = UISurface_Load_From_Playscii("assets\\ui\\sidebar.psci"); // Load up the sidebar
    sidebarSurface->x = 60;
    UI_Add_Surface(e->ui, sidebarSurface);              // Add the surface to the UI
    UIOverlay *textOverlay = UIOverlay_Create();        // Create a new overlay
    UISurface_Add_Overlay(sidebarSurface, textOverlay); // Add the new overlay to the sidebar surface
    UIOverlay_Add_Element(textOverlay,
                          (UIElement *)UIText_Create_Bound(0, 0, COLOR_WHITE, COLOR_DARK_GRAY, &e->cursor.x, "Cursor x: %d", BIND_INT)); // Create text element that prints cursor x
    UIOverlay_Add_Element(textOverlay,
                          (UIElement *)UIText_Create_Bound(0, 1, COLOR_WHITE, COLOR_DARK_GRAY, &e->cursor.y, "Cursor y: %d", BIND_INT));

    /* ------------------------------------------------------------------ */
    /* 2.  simple blue “message box” with overlayed white text            */
    /* ------------------------------------------------------------------ */
    UILayer *layer = UI_Add_Layer(e->ui);

    /* a. make a 20×5 surface at (10,10) and paint every cell blue        */
    UISurface *box = UILayer_Add_Surface(layer, 20, 10, 20, 5);
    for (int i = 0; i < box->cell_count; ++i)
    {
        box->cells[i].bg = COLOR_BLUE;
        box->cells[i].fg = COLOR_BLUE;
        box->cells[i].glyph = 255; /* solid block char */
    }

    /* b. create an overlay that lives *inside* that surface              */
    UIOverlay *boxOverlay = UIOverlay_Create();
    UISurface_Add_Overlay(box, boxOverlay);

    /* c. drop a single static UIText element at (1,1) inside the box     */
    UIText *msg = UIText_Create(
        1, 1,                                           /* position relative to the box */
        COLOR_WHITE, COLOR_BLUE,                        /* fg / bg                       */
        caption_cb,                                     /* callback: just return ud      */
        "Hello, world world world world world world!"); /* ud: literal C-string          */
    msg->wrap = true;

    UIOverlay_Add_Element(boxOverlay, (UIElement *)msg);

    // UI_Add_Surface(e->ui, textLayer);

    // UI_Print_Screen(e->ui, textLayer, COLOR_WHITE, COLOR_DARK_GRAY, true, 0, 25, "%d, %d", cx, cy);
    //  // Programmatically make a sidebar - not efficent or permananet but cool
    //  UI *ui = e->ui;

    // // Create a gray bg
    // int sidebar_w = 20;
    // int sidebar_h = 25;
    // UISurface *sidebar = UISurface_Create(sidebar_w * sidebar_h);
    // sidebar->w = sidebar_w;
    // sidebar->h = sidebar_h;
    // sidebar->x = 60;
    // sidebar->y = 0;

    // for (int i = 0; i < sidebar_w * sidebar_h; ++i)
    // {
    //     sidebar->cells[i].bg = COLOR_CYAN;
    //     sidebar->cells[i].glyph = 255;
    //     sidebar->cells[i].fg = COLOR_WHITE;
    //     sidebar->cells[i].visible = true;
    // }

    // UI_Add_Surface(ui, sidebar);

    // // Show text on it
    // // UIOverlay *o = UIOverlay_Create();
    // // UIText *t = UIText_Create(0, 0, COLOR_WHITE, COLOR_DARK_GRAY, "Sample text", NULL);
    // // UIOverlay_Add_Element(o, &t->base);
    // // UISurface_Add_Overlay(sidebar, o);
    // int x = sidebar->x;
    // int y = sidebar->y;
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
