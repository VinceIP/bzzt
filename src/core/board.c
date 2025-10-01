
#include <stdlib.h>
#include <string.h>
#include "bzzt.h"

#define START_CAP 64

Bzzt_Board *Bzzt_Board_Create(const char *name, int w, int h)
{
    Bzzt_Board *b = malloc(sizeof(Bzzt_Board));
    if (!b)
        return NULL;
    b->width = w;
    b->height = h;
    b->object_cap = START_CAP;
    b->objects = malloc(sizeof(Bzzt_Object *) * START_CAP);
    b->name = strdup(name ? name : "Untitled");
    b->object_count = 0;
    b->object_next_id = 1;
    return b;
}

/**
 * @brief Expand board size in memory if needed.
 *
 * @param b Target board.
 */
static int board_grow(Bzzt_Board *b)
{
    int new_cap = b->object_cap * 2;
    Bzzt_Object **tmp = realloc(b->objects, new_cap * sizeof(Bzzt_Object *));
    if (!tmp)
        return -1;

    b->objects = tmp;
    b->object_cap = new_cap;
    return 0;
}

void Bzzt_Board_Destroy(Bzzt_Board *b)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        Bzzt_Object_Destroy(b->objects[i]);
    }
    free(b->objects);
    free(b->name);
    free(b);
}

Bzzt_Object *Bzzt_Board_Add_Object(Bzzt_Board *b, Bzzt_Object *o)
{
    if (b->object_count == b->object_cap && board_grow(b) != 0)
        return NULL;

    o->id = b->object_next_id++;
    b->objects[b->object_count++] = o;
    return o;
}

void Bzzt_Board_Remove_Object(Bzzt_Board *b, int id)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        if (b->objects[i]->id == id)
        {
            Bzzt_Object_Destroy(b->objects[i]);
            b->objects[i] = b->objects[--b->object_count];
            return;
        }
    }
}

Bzzt_Object *Bzzt_Board_Get_Object(Bzzt_Board *b, int id)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        if (b->objects[i]->id == id)
        {
            return b->objects[i];
        }
    }
    return NULL;
}

Bzzt_Board *Bzzt_Board_From_ZZT_Board(ZZTworld *zw)
{
    if (!zw)
        return NULL;

    ZZTblock *block = zztBoardGetBlock(zw);
    Bzzt_Board *bzzt_board = Bzzt_Board_Create(zztBoardGetTitle(zw), block->width, block->height);

    bzzt_board->board_n = zztBoardGetBoard_n(zw);
    bzzt_board->board_s = zztBoardGetBoard_s(zw);
    bzzt_board->board_e = zztBoardGetBoard_e(zw);
    bzzt_board->board_w = zztBoardGetBoard_w(zw);

    bzzt_board->max_shots = zztBoardGetMaxshots(zw);
    bzzt_board->darkness = zztBoardGetDarkness(zw);
    bzzt_board->reenter = zztBoardGetReenter(zw);
    bzzt_board->reenter_x = zztBoardGetReenter_x(zw);
    bzzt_board->reenter_y = zztBoardGetReenter_y(zw);
    bzzt_board->time_limit = zztBoardGetTimelimit(zw);

    const char *msg = (const char *)zztBoardGetMessage(zw);
    if (msg)
    {
        strncpy(bzzt_board->message, msg, sizeof(bzzt_board->message) - 1);
        bzzt_board->message[sizeof(bzzt_board->message) - 1] = '\0';
    }

    for (int y = 0; y < bzzt_board->height; ++y)
    {
        for (int x = 0; x < bzzt_board->width; ++x)
        {
            Bzzt_Object *o = Bzzt_Object_From_ZZT_Tile(block, x, y);
            if (o)
                Bzzt_Board_Add_Object(bzzt_board, o);
        }
    }

    return bzzt_board;
}
