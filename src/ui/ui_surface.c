/**
 * @file ui_surface.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.2
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui.h"
#include "bzzt.h"
#include "debugger.h"
#include <stdlib.h>
#include <stdio.h>

UISurface *UISurface_Create(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h)
{
    Debug_Printf(LOG_UI, "Creating surface.");
    UISurface *surface = calloc(1, sizeof(UISurface));
    if (!surface)
        goto fail;

    surface->cell_count = w * h;

    surface->cells = calloc(surface->cell_count, sizeof(Bzzt_Cell));
    if (!surface->cells)
        goto fail;

    UIProperties props = {
        name, id, x, y, z, w, h, 0, visible, enabled, false, l};
    surface->properties = props;

    // Init empty cells
    for (int i = 0; i < surface->cell_count; ++i)
    {
        surface->cells[i].visible = false;
        surface->cells[i].glyph = 0;
        surface->cells[i].fg = COLOR_WHITE;
        surface->cells[i].bg = COLOR_TRANSPARENT;
    }

    surface->overlays = NULL;
    surface->overlays_cap = 1;
    surface->overlays_count = 0;

    surface->overlays = malloc(sizeof(UIOverlay *) * surface->overlays_cap);
    if (!surface->overlays)
        goto fail;

    return surface;

fail:
    if (surface)
    {
        if (surface->cells)
            free(surface->cells);
        if (surface->overlays)
            free(surface->overlays);
        free(surface);
    }
    Debug_Printf(LOG_UI, "Error creating surface with %d cells.", w * h);
    return NULL;
}

void UISurface_Destroy(UISurface *s)
{
    if (!s)
        return;

    if (s->cells)
        free(s->cells);

    if (s->overlays)
    {
        if (s->overlays_count > 0)
        {
            for (int i = 0; i < s->overlays_count; ++i)
            {
                UIOverlay_Destroy(s->overlays[i]);
            }
        }
        free(s->overlays);
    }
    if (s->properties.name)
        free(s->properties.name);
    free(s);
}

UIOverlay *UISurface_Add_New_Overlay(UISurface *s, char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, UILayout layout, UIAnchor anchor, UIAlign align, int spacing)
{
    Debug_Printf(LOG_UI, "Adding an overlay to a surface.");
    if (!s)
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
    UIOverlay *o = UIOverlay_Create(name, id, x, y, z, w, h, padding, visible, enabled, layout, anchor, align, spacing);
    if (!o)
    {
        Debug_Printf(LOG_UI, "Error creating overlay when adding to a surface.");
        return NULL;
    }
    s->overlays[s->overlays_count++] = o;
    o->surface = s;
    o->properties.parent = s;
    return o;
}

void UISurface_Update(UISurface *s)
{
    // ... (no changes in this function)
    if (!s || !s->properties.visible)
        return;
    for (int i = 0; i < s->overlays_count; ++i)
    {
        UIOverlay *o = s->overlays[i];
        UIOverlay_Update(o);
    }
}