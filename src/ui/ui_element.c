#include "ui.h"
#include "text.h"
#include "color.h"
#include "debugger.h"
#include <stdlib.h>
#include <stdio.h>

static const char *pass_through_caption(void *ud)
{
    return (const char *)ud;
}

UIElement *UIElement_Create(UIOverlay *o, char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, ElementType type)
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

    e->properties.parent = o;

    e->type = type;
    e->update = NULL; // TBD

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
        free(b);
        break;
    }

    case UI_ELEM_TEXT:
    {
        free(e);
        break;
    }
    default:
    {
        free(e);
        break;
    }
    }
}

void UIElement_Update(UIElement *e)
{
    if (e && e->update)
    {
        e->update(e);
    }
}

UIElement_Text *UIText_Create(UIElement *e, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *(*cb)(void *ud), void *ud)
{
    UIElement_Text *t = malloc(sizeof(UIElement_Text));
    if (!t)
        Debug_Printf(LOG_UI, "Error allocating UIText.");

    t->base = e;
    t->fg = fg;
    t->bg = bg;
    t->textCallback = cb;
    t->ud = ud;
    t->wrap = wrap;

    return t;
}

UIElement_Text *UIText_Create_Bound(int x, int y, Color_Bzzt fg, Color_Bzzt bg, const void *ptr, const void *fmt, BindType type)
{
    TextBinding *b = UIBinding_Text_Create(ptr, fmt, type);
    if (!b)
        return NULL;
    return UIText_Create(x, y, fg, bg, UIBinding_Text_Format, b);
}

UIButton *UIButton_Create(int x, int y, const char *caption, UIButtonAction cb, void *ud)
{
    UIButton *b = malloc(sizeof(UIButton));
    if (!b)
        Debug_Printf(LOG_UI, "Error allocating UIButton.");

    b->base.type = UI_ELEM_BUTTON;
    b->base.visible = true;
    b->base.x = x;
    b->base.y = y;
    b->base.update = NULL;

    b->onClick = cb;
    b->ud = ud;

    b->label = UIText_Create(0, 0, COLOR_WHITE, COLOR_BLACK, pass_through_caption, (void *)caption);

    return b;
}
