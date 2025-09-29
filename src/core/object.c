/**
 * @file object.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-09-28
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdlib.h>
#include <string.h>
#include "bzzt.h"
#include "zzt.h"
#include "debugger.h"

// TBD: assign object ids

static void set_zzt_data_labels(Bzzt_Param *bzzt_param, ZZTtile tile)
{
    for (int i = 0; i < 3; ++i)
    {
        int data_use = zztParamDatauseGet(tile, i);
        bzzt_param->data_label[i] = data_use;
    }
}

static void bzzt_param_destroy(Bzzt_Param *param)
{
    if (!param)
        return;
    if (param->program)
        free(param->program);
    free(param);
}

// Convert zzt param data to bzzt equivalent.
static Bzzt_Param *bzzt_param_from_zzt_param(ZZTparam *zzt_param, ZZTtile tile)
{
    if (!zzt_param)
        return NULL;

    Bzzt_Param *param = malloc(sizeof(Bzzt_Param));
    if (!param)
    {
        Debug_Printf(LOG_WORLD, "Error allocating zzt object param.");
        return NULL;
    }

    param->cycle = zzt_param->cycle;
    param->x_step = zzt_param->xstep;
    param->y_step = zzt_param->ystep;

    param->data[0] = zzt_param->data[0];
    param->data[1] = zzt_param->data[1];
    param->data[2] = zzt_param->data[2];

    set_zzt_data_labels(param, tile);

    if (zzt_param->program && zzt_param->length > 0)
    {
        param->program_length = zzt_param->length;
        param->program = malloc(param->program_length + 1);
        if (param->program)
        {
            memcpy(param->program, zzt_param->program, param->program_length);
            param->program[param->program_length] = '\0'; // Add null terminator
            param->program_counter = zzt_param->instruction;
        }
        else
        {
            param->program = NULL;
            param->program_length = 0;
            param->program_counter = 0;
        }
    }

    return param;
}

Bzzt_Object *Bzzt_Object_Create(uint8_t glyph, Color_Bzzt fg, Color_Bzzt bg, int x, int y)
{
    Bzzt_Object *o = malloc(sizeof(Bzzt_Object));
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
    if (o->param)
        bzzt_param_destroy(o->param);

    free(o);
}

bool Bzzt_Object_Is_Walkable(Bzzt_Object *obj)
{
    if (!obj)
        return true;

    switch (obj->bzzt_type)
    {
    case ZZT_EMPTY:
    case ZZT_FAKE:
    case ZZT_WATER:
    case ZZT_FOREST:
        return true;
        break;

    case ZZT_SOLID:
    case ZZT_NORMAL:
    case ZZT_EDGE:
    case ZZT_BOULDER:
        return false;
        break;
    default:
        return false;
        break;
    }
}

Interaction_Type Bzzt_Object_Get_Interaction_Type(Bzzt_Object *obj)
{
    if (!obj)
        return INTERACTION_NONE;

    switch (obj->bzzt_type)
    {
    case ZZT_KEY:
        return INTERACTION_KEY;
        break;
    case ZZT_DOOR:
        return INTERACTION_DOOR;
        break;
    case ZZT_GEM:
        return INTERACTION_GEM;
        break;
    case ZZT_AMMO:
        return INTERACTION_AMMO;
        break;
    case ZZT_TORCH:
        return INTERACTION_TORCH;
        break;
    case ZZT_BOULDER:
    case ZZT_NSSLIDER:
    case ZZT_EWSLIDER:
        return INTERACTION_BOULDER_PUSH;
        break;
    default:
        return INTERACTION_NONE;
        break;
    }
}

const char *Bzzt_Object_Get_Type_Name(Bzzt_Object *obj)
{
    if (!obj)
        return NULL;

    switch (obj->bzzt_type)
    {
    case ZZT_AMMO:
        return "ZZT_AMMO";
        break;
    case ZZT_SOLID:
        return "ZZT_SOLID";
        break;
    case ZZT_FAKE:
        return "ZZT_FAKE";
        break;
    case ZZT_BREAKABLE:
        return "ZZT_BREAKABLE";
        break;
    case ZZT_WATER:
        return "ZZT_WATER";
        break;
    case ZZT_EDGE:
        return "ZZT_EDGE";
        break;
    case ZZT_FOREST:
        return "ZZT_FOREST";
        break;
    case ZZT_EMPTY:
        return "ZZT_EMPTY";
        break;
    default:
        return "UNKOWN TYPE";
        break;
    }
}

// Parse a ZZT tile and convert it to a Bzzt Object.
Bzzt_Object *Bzzt_Object_From_ZZT_Tile(ZZTblock *block, int x, int y)
{
    if (!block || x >= block->width || y >= block->height)
        return NULL;

    // Get tile visual layer
    uint8_t ch = zztTileGetDisplayChar(block, x, y);
    uint8_t attr = zztTileGetDisplayColor(block, x, y);
    uint8_t fg_idx = attr & 0x0F;
    uint8_t bg_idx = (attr >> 4) & 0x0F;

    Color_Bzzt fg = bzzt_get_color(fg_idx);
    Color_Bzzt bg = bzzt_get_color(bg_idx);

    Bzzt_Object *o = Bzzt_Object_Create(ch, fg, bg, x, y);

    o->dir = DIR_NONE;

    // Get tile data layer
    ZZTtile tile = zztTileAt(block, x, y);
    uint8_t type = tile.type;
    ZZTparam *param = tile.param;

    o->bzzt_type = tile.type;

    if (param)
    {
        o->param = bzzt_param_from_zzt_param(param, tile);
        o->under_type = param->utype;
        o->under_color = param->ucolor;
    }
    else
    {
        o->param = NULL;
        o->under_type = ZZT_EMPTY;
        o->under_color = 0;
    }

    o->is_bzzt_exclusive = false;
    return o;
}
