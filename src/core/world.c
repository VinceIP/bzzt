#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"
#include "bzzt.h"
#include "input.h"
#include "color.h"
#include "debugger.h"
#include "zzt.h"

static bool grow_boards_array(Bzzt_World *w)
{
    int old_cap = w->boards_cap;
    int new_cap = w->boards_cap * 2;
    Bzzt_Board **tmp = realloc(w->boards, new_cap * sizeof(Bzzt_Board *));
    if (!tmp)
    {
        Debug_Printf(LOG_ENGINE, "Error reallocating boards array.");
        return false;
    }
    w->boards = tmp;
    for (int i = old_cap; i < new_cap; ++i)
    {
        w->boards[i] = NULL;
    }
    w->boards_cap = new_cap;
    return true;
}

static Bzzt_Object *get_object_at(Bzzt_Board *board, int x, int y)
{
    if (!board || x >= board->width || x < 0 || y > board->height || y < 0)
        return NULL;

    for (int i = 0; i < board->object_count; ++i)
    {
        if (board->objects[i]->x == x && board->objects[i]->y == y)
        {
            return board->objects[i];
        }
    }
    return NULL;
}

static bool handle_board_edge_move(Bzzt_World *w, int new_x, int new_y)
{
    // tbd
    if (!w)
        return false;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    uint8_t next_board_idx = 0;
    int entry_x = w->player->x;
    int entry_y = w->player->y;

    if (new_x < 0)
    {
        next_board_idx = current_board->board_w;
        entry_x = current_board->width - 1;
        entry_y = w->player->y;
    }
    else if (new_x >= current_board->width)
    {
        next_board_idx = current_board->board_e;
        entry_x = 0;
        entry_y = w->player->y;
    }
    else if (new_y < 0)
    {
        next_board_idx = current_board->board_n;
        entry_x = w->player->x;
        entry_y = current_board->height - 1;
    }
    else if (new_y >= current_board->height)
    {
        next_board_idx = current_board->board_s;
        entry_x = w->player->x;
        entry_y = 0;
    }

    if (next_board_idx == 0 || next_board_idx >= w->boards_count)
        return false;

    w->boards_current = next_board_idx;
    w->player->x = entry_x;
    w->player->y = entry_y;

    // temp add new player
    Bzzt_Board *new_board = w->boards[w->boards_current];
    w->player->id = 0;
    Bzzt_Board_Add_Object(new_board, w->player);

    return true;
}

static void move_player_to(Bzzt_World *w, int new_x, int new_y)
{
    w->player->x = new_x;
    w->player->y = new_y;
}

static bool handle_player_touch(Bzzt_World *w, Bzzt_Object *target, int dx, int dy)
{
    if (!w || !target)
        return false;

    // tbd
    Interaction_Type type = Bzzt_Object_Get_Interaction_Type(target);
    const char *type_name = Bzzt_Object_Get_Type_Name(target);
    Debug_Printf(LOG_LEVEL_DEBUG, "Player touched %s at (%d, %d).", type_name, target->x, target->y);
    switch (type)
    {
    case INTERACTION_GEM:
        Bzzt_Board_Remove_Object(w->boards[w->boards_current], target->id);
        break;
    default:
        break;
    }

    return true;
}

static void update_player_direction(Bzzt_World *w, InputState *in)
{
    if (in->dx > 0)
        w->player->dir = DIR_RIGHT;
    else if (in->dx < 0)
        w->player->dir = DIR_LEFT;
    else if (in->dy > 0)
        w->player->dir = DIR_DOWN;
    else if (in->dy < 0)
        w->player->dir = DIR_UP;
}

static bool do_player_move(Bzzt_World *w, int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return false;

    Bzzt_Board *board = w->boards[w->boards_current];
    int new_x = w->player->x + dx;
    int new_y = w->player->y + dy;

    if (new_x < 0 || new_x >= board->width || new_y < 0 || new_y >= board->height)
    {
        return handle_board_edge_move(w, new_x, new_y);
    }

    Bzzt_Object *target = get_object_at(board, new_x, new_y);

    if (!target || Bzzt_Object_Is_Walkable(target))
    {
        move_player_to(w, new_x, new_y);
        if (target)
        {
            switch (target->bzzt_type)
            {
            case ZZT_AMMO:
            case ZZT_TORCH:
            case ZZT_GEM:
                return handle_player_touch(w, target, dx, dy);
            default:
                return true;
            }
        }
    }

    return handle_player_touch(w, target, dx, dy);
}

static void update_player(Bzzt_World *w, InputState *in)
{
    if (!in->anyDirPressed || in->delayLock)
        return;

    do_player_move(w, in->dx, in->dy);
    update_player_direction(w, in);
}

Bzzt_World *Bzzt_World_Create(char *title)
{
    Bzzt_World *w = malloc(sizeof(Bzzt_World));
    if (!w)
        return NULL;
    strncpy(w->title, title, sizeof(w->title) - 1);
    w->boards_cap = 4;
    w->boards = (Bzzt_Board **)malloc(sizeof(Bzzt_Board *) * w->boards_cap); // allocate initial boards size to 4

    if (!w->boards)
    {
        Debug_Printf(LOG_ENGINE, "Error allocating bzzt boards array.");
        free(w);
        return NULL;
    }

    for (int i = 0; i < w->boards_cap; ++i)
    {
        w->boards[i] = NULL;
    }

    w->boards[0] = Bzzt_Board_Create("Title Screen", BZZT_BOARD_DEFAULT_W, BZZT_BOARD_DEFAULT_H); // create a starting empty title screen board

    // w->player = Bzzt_Board_Add_Object(w->boards[0], // Pushes a default player obj to the board
    //                                   Bzzt_Object_Create(2, COLOR_WHITE, COLOR_BLUE, 47, 10));

    w->boards_current = 0;
    w->boards_count = 1;
    w->doUnload = false;
    w->loaded = true;
    return w;
}

void Bzzt_World_Destroy(Bzzt_World *w)
{
    if (!w)
        return;
    for (int i = 0; i < w->boards_count; ++i)
    {
        if (w->boards[i])
        {
            Bzzt_Board_Destroy(w->boards[i]);
            w->boards[i] = NULL;
        }
    }
    w->boards_count = 0;
    w->boards_current = 0;
    w->doUnload = false;
    w->loaded = false;
    w->player = NULL;
    free(w->boards);
    free(w);
}

void Bzzt_World_Update(Bzzt_World *w, InputState *in)
{
    if (w->doUnload)
    {
        Bzzt_World_Destroy(w);
        return;
    }

    if (w->player)
        update_player(w, in);
}

void Bzzt_World_Add_Board(Bzzt_World *w, Bzzt_Board *b)
{
    if (w->boards_count >= w->boards_cap)
    {
        if (!grow_boards_array(w))
            return;
    }
    w->boards[w->boards_count++] = b;
}

Bzzt_World *Bzzt_World_From_ZZT_World(char *file)
{
    if (!file)
    {
        Debug_Printf(LOG_ENGINE, "Invalid ZZT world.");
        return NULL;
    }

    ZZTworld *zw = zztWorldLoad(file);
    if (!zw)
    {
        Debug_Printf(LOG_ENGINE, "Error loading ZZT world %s", file);
        return NULL;
    }

    Bzzt_World *bw = Bzzt_World_Create((char *)zztWorldGetTitle(zw));
    strncpy(bw->file_path, file, sizeof(bw->file_path) - 1);
    strncpy(bw->author, "Blank", sizeof(bw->author) - 1);

    // Remove default title screen created by Bzzt_World_Create
    if (bw->boards_count > 0 && bw->boards[0])
    {
        Bzzt_Board_Destroy(bw->boards[0]);
        bw->boards[0] = NULL;
        bw->boards_count = 0;
        bw->boards_current = 0;
        bw->player = NULL;
    }

    int boardCount = zztWorldGetBoardcount(zw);

    for (int i = 0; i < boardCount; ++i)
    {
        zztBoardSelect(zw, i);
        Bzzt_Board *b = Bzzt_Board_From_ZZT_Board(zw);
        Bzzt_World_Add_Board(bw, b);
    }

    bw->boards_current = (int)zztWorldGetStartboard(zw);
    bw->start_board = bw->boards[bw->boards_current];

    // Instantiate player object on start board
    Bzzt_Board *starting_board = bw->boards[bw->boards_current];
    for (int i = 0; i < starting_board->object_count; ++i)
    {
        if (starting_board->objects[i]->bzzt_type == ZZT_PLAYER)
        {
            bw->player = starting_board->objects[i];
            break;
        }
    }

    if (!bw->player)
        Debug_Printf(LOG_WORLD, "Warning: No player found in ZZT world.");

    zztWorldFree(zw);

    return bw;
}
