/**
 * @file overlay.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.2
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "debugger.h"
#include "color.h"
#include "ui.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

UIOverlay *UIOverlay_Create(char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, UILayout layout, UIAnchor anchor, UIAlign align, int spacing)
{
    Debug_Printf(LOG_UI, "Creating a new overlay.");
    UIOverlay *o = calloc(1, sizeof(UIOverlay));
    if (!o)
    {
        Debug_Printf(LOG_UI, "Error allocating UIOverlay.");
        return NULL;
    }
    o->properties.name = name;
    o->properties.id = id;
    o->properties.x = x;
    o->properties.y = y;
    o->properties.z = z;
    o->properties.w = w;
    o->properties.h = h;
    o->properties.padding = padding;
    o->properties.visible = visible;
    o->properties.enabled = enabled;
    o->properties.expand = false;
    o->properties.parent = NULL;

    o->layout = layout;
    o->anchor = anchor;
    o->align = align;
    o->spacing = spacing;

    o->elements_cap = 1;
    o->elements_count = 0;
    o->elements = calloc(o->elements_cap, sizeof(UIElement *) * o->elements_cap);
    if (!o->elements)
    {
        free(o);
        Debug_Printf(LOG_UI, "Error allocating elements array for UIOverlay.");
        return NULL;
    }

    return o;
}
static void remove_from_surface(UISurface *s, UIOverlay *o)
{
    if (!s)
        return;
    for (int i = 0; i < s->overlays_count; ++i)
    {
        if (s->overlays[i] == o)
        {
            s->overlays[i] = s->overlays[--s->overlays_count];
            return;
        }
    }
}

void UIOverlay_Destroy(UIOverlay *o)
{
    if (!o)
        return;

    if (o->elements)
    {
        for (int i = 0; i < o->elements_count; ++i)
        {
            UIElement_Destroy(o->elements[i]);
        }
        free(o->elements);
    }

    if (o->surface)
        remove_from_surface(o->surface, o);

    if (o->properties.name)
        free(o->properties.name);

    free(o);
}

void UIOverlay_Add_New_Element(UIOverlay *o, UIElement *e)
{
    if (!o || !e)
        return;

    if (o->elements_count >= o->elements_cap)
    {
        int new_cap = o->elements_cap == 0 ? 4 : o->elements_cap * 2;
        UIElement **new_elements = realloc(o->elements, sizeof(UIElement *) * new_cap);
        if (!new_elements)
        {
            Debug_Printf(LOG_UI, "Error allocating UIElements array when adding element to an overlay.");
            return;
        }
        o->elements = new_elements;
        o->elements_cap = new_cap;
    }
    o->elements[o->elements_count++] = e;
}

void UIOverlay_Update(UIOverlay *o)
{
    if (!o || !o->properties.visible)
        return;

    for (int i = 0; i < o->elements_count; ++i)
    {
        UIElement *e = o->elements[i];
        UIElement_Update(e);
    }
}

void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *fmt, ...)
{
    if (!ov || !ov->surface)
        return;

    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // This function might need updating if used, as its concept of drawing is now simpler.
    // For now, leaving as is.
    // UISurface_DrawText(ov->surface, buffer, ov->properties.x, ov->properties.y,
    //                    fg, bg, wrap, ov->surface->properties.w);
}

UIOverlay *UIOverlay_Find_By_Name(UI *ui, const char *name)
{
    if (!ui || !name)
        return NULL;

    for (int i = 0; i < ui->layer_count; ++i)
    {
        UILayer *layer = ui->layers[i];
        for (int j = 0; j < layer->surface_count; ++j)
        {
            UISurface *surface = layer->surfaces[j];
            for (int k = 0; k < surface->overlays_count; ++k)
            {
                UIOverlay *overlay = surface->overlays[k];
                if (overlay && overlay->properties.name && strcmp(overlay->properties.name, name) == 0)
                {
                    return overlay;
                }
            }
        }
    }
    Debug_Log(LOG_LEVEL_ERROR, LOG_UI, "Failed to find a UIOverlay named '%s'", name);
    return NULL;
}