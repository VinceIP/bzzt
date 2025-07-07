#include <stdlib.h>
#include "object.h"

Object *object_create(uint8_t glyph, Color_bzzt fg_color, Color_bzzt bg_color, int x, int y){
    Object *o = (Object *)malloc(sizeof(Object));
    if(!o) return NULL;
    o->glyph = glyph;
    o->fg_color  = fg_color;
    o->bg_color = bg_color;
    o->x = x;
    o->y = y;

    return o;
}