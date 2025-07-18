#include "ui.h"
#include "cell.h"
#include "debugger.h"
#include <stdlib.h>
#include <stdio.h>

UISurface *UISurface_Create(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h)
{
    Debug_Printf(LOG_UI, "Creating surface.");
    UISurface *surface = malloc(sizeof(UISurface));
    if (!surface)
        goto fail;

    UIProperties props = {
        name, id, x, y, z, w, h, 0, visible, enabled, l};
    surface->properties = props;

    surface->cell_count = w * h;
    surface->cells = malloc(sizeof(Cell) * surface->cell_count);
    if (!surface->cells)
        goto fail;

    // Init empty cells
    for (int i = 0; i < surface->cell_count; ++i)
    {
        surface->cells[i].visible = false;
        surface->cells[i].glyph = 0;
        surface->cells[i].fg = COLOR_BLACK;
        surface->cells[i].bg = COLOR_BLACK;
    }

    surface->overlays = NULL;
    surface->overlays_count = 0;
    surface->overlays_cap = 0;
    return surface;

fail:
    Debug_Printf(LOG_UI, "Error creating surface with %d cells.", cell_count);
    return NULL;
}

void UISurface_Destroy(UISurface *s)
{
    if (!s)
        return;

    if (s->cell_count > 0)
    {
        free(s->cells);
    }

    if (s->overlays && s->overlays_count > 0)
    {
        for (int i = 0; i < s->overlays_count; ++i)
        {
            UIOverlay *o = s->overlays[i];
            UIOverlay_Destroy(o);
        }
        free(s->overlays);
    }

    free(s);
}

void UISurface_Add_New_Overlay(UISurface *s, UIOverlay *o)
{
    Debug_Printf(LOG_UI, "Adding an overlay to a surface.");
    if (!s || !o)
        return;

    if (s->overlays_count >= s->overlays_cap)
    {
        int new_cap = s->overlays_cap == 0 ? 4 : s->overlays_cap * 2;
        UIOverlay **new_overlays = realloc(s->overlays, sizeof(UIOverlay *) * new_cap);
        if (!new_overlays)
        {
            Debug_Printf(LOG_UI, "Error reallocate overlays array when adding an overlay to a surface.");
            return;
        }
        s->overlays = new_overlays;
        s->overlays_cap = new_cap;
    }
    o->surface = s;
    s->overlays[s->overlays_count++] = o;
}

void UISurface_Update(UISurface *s)
{
    if (!s || !s->visible)
        return;

    for (int i = 0; i < s->overlays_count; ++i)
    {
        UIOverlay *o = s->overlays[i];
        UIOverlay_Update(o);
    }
}
