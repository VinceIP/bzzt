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
    case ZZT_GEM:
    case ZZT_AMMO:
    case ZZT_TORCH:
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
        return "NULL";

    switch (obj->bzzt_type)
    {
    case ZZT_EMPTY:
        return "Empty";
    case ZZT_EDGE:
        return "Board Edge";
    case ZZT_MESSAGETIMER:
        return "Message Timer";
    case ZZT_MONITOR:
        return "Monitor";
    case ZZT_PLAYER:
        return "Player";
    case ZZT_AMMO:
        return "Ammo";
    case ZZT_TORCH:
        return "Torch";
    case ZZT_GEM:
        return "Gem";
    case ZZT_KEY:
        return "Key";
    case ZZT_DOOR:
        return "Door";
    case ZZT_SCROLL:
        return "Scroll";
    case ZZT_PASSAGE:
        return "Passage";
    case ZZT_DUPLICATOR:
        return "Duplicator";
    case ZZT_BOMB:
        return "Bomb";
    case ZZT_ENERGIZER:
        return "Energizer";
    case ZZT_STAR:
        return "Star";
    case ZZT_CWCONV:
        return "Clockwise Conveyor";
    case ZZT_CCWCONV:
        return "Counter-Clockwise Conveyor";
    case ZZT_BULLET:
        return "Bullet";
    case ZZT_WATER:
        return "Water";
    case ZZT_FOREST:
        return "Forest";
    case ZZT_SOLID:
        return "Solid Wall";
    case ZZT_NORMAL:
        return "Normal Wall";
    case ZZT_BREAKABLE:
        return "Breakable Wall";
    case ZZT_BOULDER:
        return "Boulder";
    case ZZT_NSSLIDER:
        return "North-South Slider";
    case ZZT_EWSLIDER:
        return "East-West Slider";
    case ZZT_FAKE:
        return "Fake Wall";
    case ZZT_INVISIBLE:
        return "Invisible Wall";
    case ZZT_BLINK:
        return "Blinkwall";
    case ZZT_TRANSPORTER:
        return "Transporter";
    case ZZT_LINE:
        return "Line";
    case ZZT_RICOCHET:
        return "Ricochet";
    case ZZT_BLINKHORIZ:
        return "Horizontal Blinkwall Ray";
    case ZZT_BEAR:
        return "Bear";
    case ZZT_RUFFIAN:
        return "Ruffian";
    case ZZT_OBJECT:
        return "Object";
    case ZZT_SLIME:
        return "Slime";
    case ZZT_SHARK:
        return "Shark";
    case ZZT_SPINNINGGUN:
        return "Spinning Gun";
    case ZZT_PUSHER:
        return "Pusher";
    case ZZT_LION:
        return "Lion";
    case ZZT_TIGER:
        return "Tiger";
    case ZZT_BLINKVERT:
        return "Vertical Blinkwall Ray";
    case ZZT_CENTHEAD:
        return "Centipede Head";
    case ZZT_CENTBODY:
        return "Centipede Segment";
    case ZZT_CUSTOMTEXT:
        return "Custom Text";
    case ZZT_BLUETEXT:
        return "Blue Text";
    case ZZT_GREENTEXT:
        return "Green Text";
    case ZZT_CYANTEXT:
        return "Cyan Text";
    case ZZT_REDTEXT:
        return "Red Text";
    case ZZT_PURPLETEXT:
        return "Purple Text";
    case ZZT_YELLOWTEXT:
        return "Yellow Text";
    case ZZT_WHITETEXT:
        return "White Text";

    // Extended text types (Super ZZT compatibility tbd)
    case ZZT_BBLUETEXT:
        return "Bright Blue Text";
    case ZZT_BGREENTEXT:
        return "Bright Green Text";
    case ZZT_BCYANTEXT:
        return "Bright Cyan Text";
    case ZZT_BREDTEXT:
        return "Bright Red Text";
    case ZZT_BPURPLETEXT:
        return "Bright Purple Text";
    case ZZT_BYELLOWTEXT:
        return "Bright Yellow Text";
    case ZZT_BWHITETEXT:
        return "Bright White Text";

    default:
        return "Unknown";
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
