
#include <stdlib.h>
#include <string.h>
#include "bzzt.h"
#include "debugger.h"

#define START_CAP 16

Bzzt_Board *Bzzt_Board_Create(const char *name, int w, int h)
{
    Bzzt_Board *b = malloc(sizeof(Bzzt_Board));

    if (!b)
        return NULL;

    b->width = w;
    b->height = h;

    b->stat_cap = START_CAP;
    b->stat_count = 0;
    b->stats = malloc(sizeof(Bzzt_Stat *) * START_CAP);
    if (!b->stats)
    {
        Debug_Log(LOG_LEVEL_ERROR, LOG_BOARD, "Failed to allocate stats while creating board '%s'", name);
        return NULL;
    }

    b->tiles = calloc(w * h, sizeof(Bzzt_Tile));
    if (!b->tiles)
    {
        Debug_Log(LOG_LEVEL_ERROR, LOG_BOARD, "Failed to allocate tiles while creating board '%s'", name);
        return NULL;
    }

    b->name = strdup(name ? name : "Untitled");
    return b;
}

/**
 * @brief Expand board size in memory if needed.
 *
 * @param b Target board.
 */
static int grow_stats(Bzzt_Board *b)
{
    int new_cap = b->stat_cap * 2;
    Bzzt_Stat **tmp = realloc(b->stats, new_cap * sizeof(Bzzt_Stat *));
    if (!tmp)
        return -1;

    b->stats = tmp;
    b->stat_cap = new_cap;
    return 0;
}

void Bzzt_Board_Destroy(Bzzt_Board *b)
{
    if (!b)
        return;

    // Free all stats;
    for (int i = 0; i < b->stat_count; ++i)
    {
        if (b->stats[i])
        {
            if (b->stats[i]->program)
                free(b->stats[i]->program);

            free(b->stats[i]);
        }
    }

    int len = b->width * b->height;

    free(b->stats);
    free(b->tiles);
    free(b);
}

Bzzt_Stat *Bzzt_Board_Add_Stat(Bzzt_Board *b, Bzzt_Stat *s)
{
    if (!b || !s)
        return NULL;

    if (b->stat_count >= b->stat_cap && grow_stats(b) != 0)
        return NULL;

    b->stats[b->stat_count++] = s;

    return s;
}

void Bzzt_Board_Remove_Stat(Bzzt_Board *b, int idx)
{
    if (!b || idx < 0 || idx >= b->stat_count)
        return;

    Bzzt_Stat *stat = b->stats[idx];
    if (stat && stat->program)
        free(stat->program);

    for (int i = 0; i < b->stat_count; ++i)
    {
        if (b->stats[i]->follower > idx)
            b->stats[i]->follower--;
        else if (b->stats[i]->follower == idx)
            b->stats[i]->follower = -1;

        if (b->stats[i]->leader > idx)
            b->stats[i]->leader--;
        else if (b->stats[i]->leader == idx)
            b->stats[i]->leader = -1;
    }

    free(stat);

    for (int i = idx + 1; i < b->stat_count; ++i)
        b->stats[i - 1] = b->stats[i];

    b->stat_count--;
}

Bzzt_Stat *Bzzt_Board_Get_Stat_At(Bzzt_Board *b, int x, int y)
{
    if (!b)
        return NULL;

    for (int i = 0; i < b->stat_count; ++i)
    {
        if (b->stats[i]->x == x && b->stats[i]->y == y)
            return b->stats[i];
    }
    return NULL;
}

int Bzzt_Board_Get_Stat_Index(Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!b || !stat)
        return -1;

    for (int i = 0; i < b->stat_count; ++i)
    {
        if (b->stats[i] == stat)
            return i;
    }
    return -1;
}

Bzzt_Tile Bzzt_Board_Get_Tile(Bzzt_Board *b, int x, int y)
{
    if (!b || x < 0 || x >= b->width || y < 0 || y >= b->height)
    {
        Bzzt_Tile dud = {0};
        return dud;
    }

    return b->tiles[y * b->width + x];
}

bool Bzzt_Board_Set_Tile(Bzzt_Board *b, int x, int y, Bzzt_Tile tile)
{
    if (!b || x < 0 || x >= b->width || y < 0 || y >= b->height)
        return false;

    b->tiles[y * b->width + x] = tile;
    return true;
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

    // Populate tiles
    for (int y = 0; y < bzzt_board->height; ++y)
    {
        for (int x = 0; x < bzzt_board->width; ++x)
        {
            Bzzt_Tile tile = Bzzt_Tile_From_ZZT_Tile(block, x, y);
            Bzzt_Board_Set_Tile(bzzt_board, x, y, tile);
        }
    }

    if (block->params && block->paramcount > 0)
    {
        for (int i = 0; i < block->paramcount; ++i)
        {
            ZZTparam *param = block->params[i];
            if (param)
            {
                ZZTtile tile = zztTileAt(block, param->x, param->y);
                Bzzt_Stat *stat = Bzzt_Stat_From_ZZT_Param(param, tile, param->x, param->y);
                if (stat)
                    Bzzt_Board_Add_Stat(bzzt_board, stat);
            }
        }
    }

    return bzzt_board;
}
