/**
 * @file ui_element.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-07
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui.h"
#include "color.h"
#include "debugger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// unused
UIElement *UIElement_Create(UIOverlay *o, char *name, int id, int x, int y, int z, int w, int h,
                            int padding, Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled,
                            bool expand, UIAlign align, ElementType type)

{
    UIElement *e = calloc(1, sizeof(UIElement));
    if (!e)
        goto fail;

    e->properties.name = name;
    e->properties.id = id;
    e->properties.x = x;
    e->properties.y = y;
    e->properties.z = z;
    e->properties.w = w;
    e->properties.h = h;
    e->properties.padding = padding;
    e->properties.visible = visible;
    e->properties.enabled = enabled;
    e->properties.expand = expand;
    e->properties.parent = o;
    e->properties.align = align;
    e->cell_count = e->properties.w * e->properties.h;
    e->child = NULL;

    e->type = type;
    return e;

fail:
    Debug_Printf(LOG_UI, "Error allocating UIElement.");
    return NULL;
}

void UIElement_Destroy(UIElement *e)
{
    if (!e)
        return;

    switch (e->type)
    {
    case UI_ELEM_BUTTON:
    {
        UIButton *b = (UIButton *)e;

        if (b->events.on_down_action)
        {
            free(b->events.on_down_action);
            b->events.on_down_action = NULL;
        }
        if (b->events.on_press_action)
        {
            free(b->events.on_press_action);
            b->events.on_press_action = NULL;
        }
        if (b->events.on_release_action)
        {
            free(b->events.on_release_action);
            b->events.on_release_action = NULL;
        }

        // Free label and name
        if (b->label)
            UIElement_Destroy((UIElement *)b->label);
        if (b->base.properties.name)
            free(b->base.properties.name);

        free(b);
        break;
    }

    case UI_ELEM_TEXT:
    {
        UIElement_Text *t = (UIElement_Text *)e;
        if (t->ud && t->owns_ud)
            free(t->ud);
        if (e->properties.name)
            free(e->properties.name);
        free(t);
        break;
    }
    default:
    {
        if (e->properties.name)
            free(e->properties.name);
        free(e);
        break;
    }
    }
}

void UIElement_Update(UIElement *e)
{
}

// Needs fixed
UIElement_Text *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, UIAlign align,
                              const char *(*cb)(void *ud), void *ud, bool owns_ud)
{

    {
        UIElement_Text *t = calloc(1, sizeof(UIElement_Text));
        if (!t)
        {
            Debug_Printf(LOG_UI, "Error allocating UIText.");
            return NULL;
        }

        // initialize base element
        t->base.type = UI_ELEM_TEXT;
        t->base.properties.name = NULL;
        t->base.properties.id = 0;
        t->base.properties.x = x;
        t->base.properties.y = y;
        t->base.properties.z = 0;
        t->base.properties.w = 0;
        t->base.properties.h = 0;
        t->base.properties.padding = 0;
        t->base.properties.visible = true;
        t->base.properties.enabled = true;
        t->base.properties.expand = false;
        t->base.properties.parent = NULL;
        t->base.properties.align = align;
        t->fg = fg;
        t->bg = bg;
        t->textCallback = cb;
        t->ud = ud;
        t->wrap = wrap;
        t->owns_ud = owns_ud;

        return t;
    }
}

// needs fixed
UIElement_Text *UIText_Create_Bound(int x, int y, Color_Bzzt fg, Color_Bzzt bg, const void *ptr, const void *fmt, BindType type)
{
    TextBinding *b = UIBinding_Text_Create(ptr, fmt, type);
    if (!b)
        return NULL;
    return UIText_Create(x, y, fg, bg, false, ALIGN_LEFT, UIBinding_Text_Format, b, b->on_heap);
}

UIButton *UIButton_Create(UIOverlay *o, const char *name, int id, int x, int y, int z, int w, int h, int padding,
                          Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled, bool expand, UIAlign align,
                          const char *caption, UIButtonAction cb, void *ud)

{
    UIButton *b = calloc(1, sizeof(UIButton));
    if (!b)
    {
        Debug_Printf(LOG_UI, "Error allocating UIButton.");
        return NULL;
    }

    b->base.type = UI_ELEM_BUTTON;
    b->base.properties.name = name ? strdup(name) : NULL;
    b->base.properties.id = id;
    b->base.properties.x = x;
    b->base.properties.y = y;
    b->base.properties.z = z;
    b->base.properties.w = w;
    b->base.properties.h = h;
    b->base.properties.align = align;
    b->base.properties.enabled = enabled;
    b->base.properties.visible = visible;
    b->base.properties.expand = false;
    b->base.properties.parent = o;

    b->onClick = cb;
    b->ud = ud;

    b->label = UIText_Create(0, 0, COLOR_WHITE, COLOR_TRANSPARENT, false, align, pass_through_caption, caption, true);
    if (!b->label)
    {
        if (b->base.properties.name)
            free(b->base.properties.name);
        free(b);
        return NULL;
    }
    b->label->base.properties.parent = b;
    return b;
}

UIElement *UIElement_Find_By_Name(UI *ui, const char *name)
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
                for (int l = 0; l < overlay->elements_count; ++l)
                {
                    UIElement *elem = overlay->elements[l];
                    if (elem && elem->properties.name && strcmp(elem->properties.name, name) == 0)
                        return elem;
                }
            }
        }
    }

    Debug_Log(LOG_LEVEL_ERROR, LOG_UI, "Failed to find a UIElement named '%s'", name);
    return NULL;
}

void UIElement_Set_Enabled(UIElement *elem, bool enabled)
{
    if (!elem)
        return;
    elem->properties.enabled = enabled;
}
void UIElement_Set_Visible(UIElement *elem, bool visible)
{
    if (!elem)
        return;
    elem->properties.visible = visible;
}