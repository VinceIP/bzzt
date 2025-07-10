#include <stdlib.h>
#include "object.h"

Object *Object_Create(uint8_t glyph, Color_bzzt fg_color, Color_bzzt bg_color, int x, int y)
{
    Object *o = (Object *)malloc(sizeof(Object));
    if (!o)
        return NULL;
    o->id = 0;
    o->dir = DIR_NONE;
    o->glyph = glyph;
    o->fg_color = fg_color;
    o->bg_color = bg_color;
    o->x = x;
    o->y = y;

    return o;
}

void Object_Destroy(Object *o)
{
    if (!o)
        return;
    free(o);
}