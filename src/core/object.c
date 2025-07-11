#include <stdlib.h>
#include "object.h"

Object *Object_Create(uint8_t glyph, Color_Bzzt fg, Color_Bzzt bg, int x, int y)
{
    Object *o = (Object *)malloc(sizeof(Object));
    if (!o)
        return NULL;
    o->id = 0;
    o->dir = DIR_NONE;
    o->x = x;
    o->y = y;
    o->cell.glyph=glyph;
    o->cell.bg=bg;
    o->cell.fg=fg;
    return o;
}

void Object_Destroy(Object *o)
{
    if (!o)
        return;
    free(o);
}