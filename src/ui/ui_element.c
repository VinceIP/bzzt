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

static const char *pass_through_caption(void *ud)
{
    return (const char *)ud;
}

UIElement *UIElement_Create(UIOverlay *o, char *name, int id, int x, int y, int z, int w, int h, int padding, Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled, bool expand, ElementType type)
{
    UIElement *e = malloc(sizeof(UIElement));
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

    e->cell_count = e->properties.w * e->properties.h;

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
        if (b->label)
            UIElement_Destroy((UIElement *)b->label);
        if (e->properties.name)
            free(e->properties.name);
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
UIElement_Text *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap,
                              const char *(*cb)(void *ud), void *ud, bool owns_ud)
{
    UIElement_Text *t = malloc(sizeof(UIElement_Text));
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
    t->fg = fg;
    t->bg = bg;
    t->textCallback = cb;
    t->ud = ud;
    t->wrap = wrap;
    t->owns_ud = owns_ud;

    return t;
}

// needs fixed
UIElement_Text *UIText_Create_Bound(int x, int y, Color_Bzzt fg, Color_Bzzt bg, const void *ptr, const void *fmt, BindType type)
{
    TextBinding *b = UIBinding_Text_Create(ptr, fmt, type);
    if (!b)
        return NULL;
    return UIText_Create(x, y, fg, bg, false, UIBinding_Text_Format, b, b->on_heap);
}

UIButton *UIButton_Create(UIOverlay *o, const char *name, int id, int x, int y, int z, int w, int h, int padding, Color_Bzzt fg,
                          Color_Bzzt bg, bool visible, bool enabled, bool expand, const char *caption, UIButtonAction cb,
                          void *ud)
{
    UIButton *b = malloc(sizeof(UIButton));
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
    b->base.properties.enabled = enabled;
    b->base.properties.visible = visible;
    b->base.properties.expand = false;
    b->base.properties.parent = o;

    b->onClick = cb;
    b->ud = ud;

    char *dup = caption ? strdup(caption) : strdup("");
    b->label = UIText_Create(0, 0, COLOR_WHITE, COLOR_TRANSPARENT, false, pass_through_caption, dup, true);
    if (!b->label)
    {
        if (dup)
            free(dup);
        if (b->base.properties.name)
            free(b->base.properties.name);
        free(b);
        return NULL;
    }
    b->label->base.properties.parent = b;
    return b;
}