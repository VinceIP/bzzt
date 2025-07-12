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

#include "ui_overlay.h"
#include "color.h"
#include "text.h"
#include "ui.h"
#include "ui_surface.h"
#include "ui_element.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

UIOverlay *UIOverlay_Create()
{
    UIOverlay *o = malloc(sizeof(UIOverlay));
    if (!o)
    {
        fprintf(stderr, "Error allocating UIOverlay.");
        return;
    }
    o->elements = NULL;
    o->elements_count = o->elements_cap = 0;
    o->visible = true;
    o->x = o->y = o->z = 0;

    return o;
}

void UIOverlay_Destroy(UIOverlay *o)
{
    if (!o)
        return;
    UIElement *elements = o->elements;
    int elements_count = o->elements_count;
    if (elements && o->elements_count > 0)
    {
        for (int i = 0; i < elements_count; ++i)
        {
            UIElement *e = o->elements[i];
            UIElement_Destroy(e);
        }
        free(o->elements);
    }
    free(o);
}

void UIOverlay_Add_Element(UIOverlay *o, UIElement *e)
{
    if (!o || !e)
        return;

    if (o->elements_count >= o->elements_cap)
    {
        int new_cap = o->elements_cap == 0 ? 4 : o->elements_cap * 2;
        UIElement **new_elements = realloc(o->elements, sizeof(UIElement *) * new_cap);
        if (!new_elements)
        {
            fprintf(stderr, "Failed to reallocate UIElements array.");
            return;
        }
        o->elements = new_elements;
        o->elements_cap = new_cap;
    }
    o->elements[o->elements_count++] = e;
}

/*
void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *fmt, ...)
{
    char buffer[256]; // Increased buffer size for safety
    va_list args;     // Handle args
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args); // Format text using any args
    va_end(args);

    int absX = ov->x;
    int absY = ov->y;
    UISurface *s = ov->surface;

    UIText_WriteRaw(ov->surface, buffer, absX, absY, fg, bg, wrap, s->w);
}
    */