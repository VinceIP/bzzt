#include <stdlib.h>
#include "bzzt.h"

Bzzt_Object *Bzzt_Object_Create(uint8_t glyph, Color_Bzzt fg, Color_Bzzt bg, int x, int y)
{
    Bzzt_Object *o = (Bzzt_Object *)malloc(sizeof(Bzzt_Object));
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

void Bzzt_Object_Destroy(Bzzt_Object *o)
{
    if (!o)
        return;
    free(o);
}