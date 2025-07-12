#include "ui_surface.h"
#include "ui_overlay.h"
#include "cell.h"
#include <stdlib.h>
#include <stdio.h>

UISurface *UISurface_Create(int cell_count)
{
    UISurface *surface = malloc(sizeof(UISurface));
    if (!surface)
        goto fail;

    surface->visible = true;
    surface->x = surface->y = surface->z = 0;
    surface->w = surface->h = 0;
    surface->cells = malloc(sizeof(Cell) * cell_count);
    if (!surface->cells)
        goto fail;
    surface->cell_count = cell_count;

    for (int i = 0; i < cell_count; ++i)
    {
        surface->cells[i].visible = true;
        surface->cells[i].glyph = 255;
        surface->cells[i].fg = COLOR_WHITE;
        surface->cells[i].bg = COLOR_BLACK;
    }
    surface->overlays = NULL;
    surface->overlays_count = 0;
    surface->overlays_cap = 0;
    return surface;

fail:
    fprintf(stderr, "Error creating surface with %d cells.", cell_count);
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

void UISurface_Add_Overlay(UISurface *s, UIOverlay *o)
{
    if (!s || !o)
        return;

    if (s->overlays_count >= s->overlays_cap)
    {
        int new_cap = s->overlays_cap == 0 ? 4 : s->overlays_cap * 2;
        UIOverlay **new_overlays = realloc(s->overlays, sizeof(UIOverlay *) * new_cap);
        if (!new_overlays)
        {
            fprintf(stderr, "Failed to reallocate UIOverlays array.");
            return;
        }
        s->overlays = new_overlays;
        s->overlays_cap = new_cap;
    }
    if (s->overlays_count >= s->overlays_cap)
    {
        int new_cap = s->overlays_cap == 0 ? 4 : s->overlays_cap * 2;
        UIOverlay **new_overlays = realloc(s->overlays, sizeof(UIOverlay *) * new_cap);
        if (!new_overlays)
        {
            fprintf(stderr, "Failed to reallocate UIOverlays array.");
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
