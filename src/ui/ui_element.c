#include "ui_element.h"
#include "text.h"
#include "color.h"
#include <stdlib.h>
#include <stdio.h>

static const char *pass_through_caption(void *ud)
{
    return (const char *)ud;
}

UIElement *UIElement_Create()
{
    UIElement *e = malloc(sizeof(UIElement));
    if (!e)
        goto fail;

    e->type = UI_ELEM_NONE;
    e->visible = false;
    e->x = 0;
    e->y = 0;
    e->update = NULL;
    return e;

fail:
    fprintf(stderr, "Error allocating UIElement.");
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

UIText *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg, const char *(*cb)(void *), void *ud)
{
    UIText *t = malloc(sizeof(UIText));
    if (!t)
        fprintf(stderr, "Error allocating UIText.");

    t->base.type = UI_ELEM_TEXT;
    t->base.visible = true;
    t->base.x = x;
    t->base.y = y;
    t->base.update = NULL;

    t->textCallback = cb;
    t->ud = ud;
    t->fg = fg;
    t->bg = bg;
    t->wrap = false;

    return t;
}

UIButton *UIButton_Create(int x, int y, const char *caption, UIButtonAction cb, void *ud)
{
    UIButton *b = malloc(sizeof(UIButton));
    if (!b)
        fprintf(stderr, "Error allocating UIButton.");

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
