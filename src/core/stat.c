/**
 * @file stat.c
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

static void set_zzt_data_labels(Bzzt_Stat *bzzt_param, ZZTtile tile)
{
    for (int i = 0; i < 3; ++i)
    {
        int data_use = zztParamDatauseGet(tile, i);
        bzzt_param->data_label[i] = data_use;
    }
}

static void bzzt_param_destroy(Bzzt_Stat *param)
{
    if (!param)
        return;
    if (param->program)
        free(param->program);
    free(param);
}

static bool stat_can_act(Bzzt_World *w, Bzzt_Stat *stat, int stat_idx)
{
    if (!w || !stat)
        return false;

    int current_tick = w->tick_counter;
    return (current_tick % stat->cycle) == (stat_idx % stat->cycle);
}

static void handle_bullet_collision(Bzzt_Board *b, Bzzt_Stat *bullet, int next_x, int next_y)
{
    if (!b || !bullet)
        return;

    Bzzt_Tile *next_tile = Bzzt_Board_Get_Tile(b, next_x, next_y);
    int idx = Bzzt_Board_Get_Stat_Index(b, bullet);

    switch (next_tile->element)
    {
    case ZZT_EMPTY:
    case ZZT_FAKE:
    case ZZT_WATER: // Bullet will pass over water, empty, fake
        Bzzt_Board_Move_Stat_To(b, bullet, next_x, next_y);
        break;
    case ZZT_BREAKABLE:
        Bzzt_Board_Set_Tile(b, next_x, next_y, (Bzzt_Tile){0}); // Kill breakable and bullet
        Bzzt_Board_Stat_Die(b, bullet);
        break; // pun
    case ZZT_RICOCHET:
        // tbd
        break;
    case ZZT_OBJECT:
        // tbd
        break;
    case ZZT_BULLET: // bullets destroy each other
        // tbd
        break;
    case ZZT_BEAR:
    case ZZT_CENTHEAD:
    case ZZT_CENTBODY:
    case ZZT_LION:
    case ZZT_RUFFIAN:
    case ZZT_SHARK:
    case ZZT_TIGER:
        // tbd enemy collision
        break;
    case ZZT_PLAYER:
        // tbd player collision
        break;
    default: // Hitting any other element kills the bullet
        Bzzt_Board_Stat_Die(b, bullet);
        break;
    }
}

static void stat_act(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !b || !stat)
        return;

    Bzzt_Tile *tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);
    if (!tile)
        return;

    switch (tile->element)
    {
    case ZZT_BULLET:
        int shooter = stat->data[0]; // 0 is player, otherwise int is stat index of shooter
        int next_x = stat->x + stat->step_x;
        int next_y = stat->y + stat->step_y;
        if (Bzzt_Board_Is_In_Bounds(b, next_x, next_y))
            handle_bullet_collision(b, stat, next_x, next_y);
    }

    void Bzzt_Stat_Update(Bzzt_World * w, Bzzt_Board * b)
    {
        if (!b)
            return;

        for (int i = 0; i < b->stat_count; ++i)
        {
            Bzzt_Stat *stat = b->stats[i];
            if (stat_can_act(w, stat, i))
                stat_act(w, stat);
        }
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

    void Bzzt_Object_Destroy(Bzzt_Object * o)
    {
        if (!o)
            return;
        if (o->param)
            bzzt_param_destroy(o->param);

        free(o);
    }

    bool Bzzt_Tile_Is_Walkable(Bzzt_Tile tile)
    {
        switch (tile.element)
        {
        case ZZT_EMPTY:
        case ZZT_FAKE:
        case ZZT_FOREST:
        case ZZT_GEM:
        case ZZT_AMMO:
        case ZZT_TORCH:
        case ZZT_ENERGIZER:
        case ZZT_KEY:
        case ZZT_DOOR:
            return true;
            break;

        default:
            return false;
            break;
        }
    }

    Interaction_Type Bzzt_Tile_Get_Interaction_Type(Bzzt_Tile tile)
    {
        switch (tile.element)
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

    const char *Bzzt_Tile_Get_Type_Name(Bzzt_Tile tile)
    {
        switch (tile.element)
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

    Bzzt_Tile Bzzt_Tile_From_ZZT_Tile(ZZTblock * block, int x, int y)
    {
        Bzzt_Tile tile = {0};
        if (!block || x < 0 || x > block->width || y < 0 || y > block->height)
            return tile;

        tile.glyph = zztTileGetDisplayChar(block, x, y);
        uint8_t attr = zztTileGetDisplayColor(block, x, y);

        uint8_t fg_idx = attr & 0x0F;
        uint8_t bg_idx = (attr >> 4) & 0x0F;
        bool blink = (attr & 0x80) != 0;

        tile.fg = bzzt_get_color(fg_idx);
        tile.bg = bzzt_get_color(bg_idx);
        tile.blink = blink;

        ZZTtile zzt_tile = zztTileAt(block, x, y);
        tile.element = zzt_tile.type;

        tile.visible = true;

        return tile;
    }

    Bzzt_Stat *Bzzt_Stat_From_ZZT_Param(ZZTparam * param, ZZTtile tile, int x, int y)
    {
        if (!param)
            return NULL;

        Bzzt_Stat *stat = malloc(sizeof(Bzzt_Stat));
        if (!stat)
        {
            Debug_Log(LOG_LEVEL_ERROR, LOG_ENGINE, "Error allocating stat from ZZT world.");
            return NULL;
        }

        stat->x = x;
        stat->y = y;

        stat->step_x = param->xstep;
        stat->step_y = param->ystep;
        stat->cycle = param->cycle;

        for (int i = 0; i < 3; ++i)
        {
            stat->data[i] = param->data[i];
            stat->data_label[i] = zztParamDatauseGet(tile, i);
        }

        stat->follower = param->followerindex;
        stat->leader = param->leaderindex;

        stat->under.element = param->utype;
        stat->under.glyph = zzt_type_to_cp437(param->utype, param->ucolor);

        uint8_t fg_idx = param->ucolor & 0x0F;
        uint8_t bg_idx = (param->ucolor >> 4) & 0x0F;
        bool blink = (param->ucolor & 0x80) != 0;
        stat->under.fg = bzzt_get_color(fg_idx);
        stat->under.bg = bzzt_get_color(bg_idx);
        stat->under.visible = true;
        stat->under.blink = blink;

        if (param->program && param->length > 0)
        {
            stat->program_length = param->length;
            stat->program = malloc(stat->program_length + 1);

            if (stat->program)
            {
                memcpy(stat->program, param->program, stat->program_length);
                stat->program[stat->program_length] = '\0';
                stat->program_counter = param->instruction;
            }
            else
            {
                stat->program = NULL;
                stat->program_length = 0;
                stat->program_counter = 0;
            }
        }
        else
        {
            stat->program = NULL;
            stat->program_length = 0;
            stat->program_counter = 0;
        }

        return stat;
    }
