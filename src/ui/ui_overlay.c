/**
 * @file overlay.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
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

UIOverlay *UIOverlay_Create(char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, OverlayLayout layout, OverlayAnchor anchor, int spacing)
{
    Debug_Printf(LOG_UI, "Creating a new overlay.");
    UIOverlay *o = malloc(sizeof(UIOverlay));
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

    o->layout = layout;
    o->anchor = anchor;
    o->spacing = spacing;

    o->elements_cap = 1;
    o->elements_count = 0;
    o->elements = malloc(sizeof(UIElement *) * o->elements_cap);
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

    UIElement **elements = o->elements;
    int elements_count = o->elements_count;
    if (elements && elements_count > 0)
    {
        for (int i = 0; i < elements_count; ++i)
        {
            UIElement *e = o->elements[i];
            UIElement_Destroy(e);
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

    UISurface_DrawText(ov->surface, buffer, ov->properties.x, ov->properties.y,
                       fg, bg, wrap, ov->surface->properties.w);
}