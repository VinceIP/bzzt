#include <stdlib.h>
#include "bzzt.h"
#include "zzt.h"

Bzzt_Object *Bzzt_Object_Create(uint8_t glyph, Color_Bzzt fg, Color_Bzzt bg, int x, int y)
{
    Bzzt_Object *o = (Bzzt_Object *)malloc(sizeof(Bzzt_Object));
    if (!o)
        return NULL;
    o->id = 0;
    o->dir = DIR_NONE;
    o->x = x;
    o->y = y;
    o->cell.glyph = glyph;
    o->cell.bg = bg;
    o->cell.fg = fg;
    return o;
}

void Bzzt_Object_Destroy(Bzzt_Object *o)
{
    if (!o)
        return;
    free(o);
}

Bzzt_Object *Bzzt_Object_From_ZZT_Tile(ZZTblock *block, int x, int y)
{
    unsigned char ch = zztTileGetDisplayChar(block, x, y);
    uint8_t attr = zztTileGetDisplayColor(block, x, y);
    uint8_t fg_idx = attr & 0x0F;
    uint8_t bg_idx = (attr >> 4) & 0x0F;

    Color_Bzzt fg = bzzt_get_color(fg_idx);
    Color_Bzzt bg = bzzt_get_color(bg_idx);

    Bzzt_Object *o = Bzzt_Object_Create(ch, fg, bg, x, y);

    return o;
}
