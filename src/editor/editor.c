#include <stdbool.h>
#include <stdio.h>
#include "raylib.h"
#include "editor.h"
#include "engine.h"
#include "input.h"
#include "cell.h"
#include "ui.h"
#include "text.h"
#include "camera.h"

int testX = 0;

static const char *caption_cb(void *ud) { return (const char *)ud; }

static void ui_init(Engine *e)
{
    UISurface *sidebarSurface = UISurface_Load_From_Playscii("assets\\ui\\sidebar.psci"); // Load up the sidebar
    sidebarSurface->x = 60;
    UI_Add_Surface(e->ui, sidebarSurface);              // Add the surface to the UI
    UIOverlay *textOverlay = UIOverlay_Create();        // Create a new overlay
    UISurface_Add_Overlay(sidebarSurface, textOverlay); // Add the new overlay to the sidebar surface
    UIOverlay_Add_Element(textOverlay,
                          (UIElement *)UIText_Create_Bound(2, sidebarSurface->h-2, COLOR_WHITE, COLOR_DARK_GRAY, &e->cursor.position.x, "Cursor x: %d", BIND_INT)); // Create text element that prints cursor x
    UIOverlay_Add_Element(textOverlay,
                          (UIElement *)UIText_Create_Bound(2, sidebarSurface->h-1, COLOR_WHITE, COLOR_DARK_GRAY, &e->cursor.position.y, "Cursor y: %d", BIND_INT));
}

void Editor_Init(Engine *e)
{
    e->edit_mode_init_done = true;
    Cursor *c = &e->cursor;
    c->color = COLOR_WHITE;
    c->blinkRate = 0.5;
    c->glyph = 219;
    c->position.x = 41;
    c->position.y = 14;
    c->visible = true;
    c->enabled = true;
    c->lastBlink = GetTime();

    e->camera->viewport.rect = (Rectangle){0, 0, 60, 25};

    ui_init(e);
}

void Editor_Update(Engine *e, InputState *in)
{
    
}
