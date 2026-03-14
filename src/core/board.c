
#include <stdlib.h>
#include <string.h>
#include "bzzt.h"
#include "debugger.h"
#include "zzt_element_defaults.h"

#define START_CAP 16

static Bzzt_Tile empty_tile = {0};

static int board_cell_index(Bzzt_Board *b, int x, int y)
{
    if (!b || x < 0 || x >= b->width || y < 0 || y >= b->height)
        return -1;

    return (y * b->width) + x;
}

static void board_clear_stat_index(Bzzt_Board *b)
{
    if (!b || !b->stat_index_grid)
        return;

    int cell_count = b->width * b->height;
    for (int i = 0; i < cell_count; ++i)
        b->stat_index_grid[i] = -1;
}

Bzzt_Board *Bzzt_Board_Create(const char *name, int w, int h)
{
    Bzzt_Board *b = calloc(1, sizeof(Bzzt_Board));

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
        free(b);
        return NULL;
    }

    b->tiles = calloc(w * h, sizeof(Bzzt_Tile));
    if (!b->tiles)
    {
        Debug_Log(LOG_LEVEL_ERROR, LOG_BOARD, "Failed to allocate tiles while creating board '%s'", name);
        free(b->stats);
        free(b);
        return NULL;
    }

    b->stat_index_grid = malloc(sizeof(int) * (size_t)(w * h));
    if (!b->stat_index_grid)
    {
        Debug_Log(LOG_LEVEL_ERROR, LOG_BOARD, "Failed to allocate stat index while creating board '%s'", name);
        free(b->tiles);
        free(b->stats);
        free(b);
        return NULL;
    }

    b->name = strdup(name ? name : "Untitled");
    if (!b->name)
    {
        free(b->stat_index_grid);
        free(b->tiles);
        free(b->stats);
        free(b);
        return NULL;
    }
    board_clear_stat_index(b);
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

    free(b->name);
    free(b->stat_index_grid);
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
    Bzzt_Board_Rebuild_Stat_Index(b);

    return s;
}

void Bzzt_Board_Remove_Stat(Bzzt_Board *b, int idx)
{
    if (!b || idx < 0 || idx >= b->stat_count)
        return;

    Bzzt_Stat *stat = b->stats[idx];
    if (!stat)
        return;

    if (stat->program)
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
    Bzzt_Board_Rebuild_Stat_Index(b);
}

Bzzt_Stat *Bzzt_Board_Get_Stat_At(Bzzt_Board *b, int x, int y)
{
    int cell_idx = board_cell_index(b, x, y);
    if (!b || cell_idx < 0 || !b->stat_index_grid)
        return NULL;

    int stat_idx = b->stat_index_grid[cell_idx];
    if (stat_idx < 0 || stat_idx >= b->stat_count)
        return NULL;

    return b->stats[stat_idx];
}

void Bzzt_Board_Rebuild_Stat_Index(Bzzt_Board *b)
{
    if (!b || !b->stat_index_grid)
        return;

    board_clear_stat_index(b);

    for (int i = 0; i < b->stat_count; ++i)
    {
        Bzzt_Stat *stat = b->stats[i];
        int cell_idx = board_cell_index(b, stat ? stat->x : -1, stat ? stat->y : -1);
        if (!stat || cell_idx < 0)
            continue;
        b->stat_index_grid[cell_idx] = i;
    }
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

void Bzzt_Board_Stat_Die(Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!b || !stat)
        return;

    int idx = Bzzt_Board_Get_Stat_Index(b, stat);
    if (idx < 0 || idx >= b->stat_count)
        return;

    Bzzt_Board_Set_Tile(b, stat->x, stat->y, stat->under);
    Bzzt_Board_Remove_Stat(b, idx);
}

int Bzzt_Board_Get_Bullet_Count(Bzzt_Board *b)
{
    if (!b)
        return 0;
    int count = 0;
    for (int i = 0; i < b->stat_count; ++i)
    {
        Bzzt_Stat *s = b->stats[i];
        if (!s)
            continue;
        Bzzt_Tile t = Bzzt_Board_Get_Tile(b, s->x, s->y);
        if (t.element == ZZT_BULLET)
            count++;
    }
    return count;
}

static bool projectile_can_enter_tile(Bzzt_Tile tile)
{
    switch (tile.element)
    {
    case ZZT_EMPTY:
    case ZZT_WATER:
    case ZZT_FAKE:
    case ZZT_BREAKABLE:
        return true;
    default:
        return false;
    }
}

static int get_default_glyph_for_element(uint8_t type)
{
    const ZZT_Element_Defaults *defaults = zzt_get_element_defaults(type);
    if (defaults)
        return defaults->default_glyph;
    return 0;
}

Bzzt_Stat *Bzzt_Board_Spawn_Stat(Bzzt_Board *b, uint8_t type, int x, int y, Color_Bzzt fg, Color_Bzzt bg)
{
    if (!b)
        return NULL;

    Bzzt_Stat *s = Bzzt_Stat_Create(b, x, y);
    if (!s)
        return NULL;

    Bzzt_Tile tile = {0};
    tile.element = type;
    tile.glyph = get_default_glyph_for_element(type);
    tile.fg = fg;
    tile.bg = bg;
    tile.visible = true;
    const ZZT_Element_Defaults *defaults = zzt_get_element_defaults(type);

    s->cycle = defaults->default_cycle;
    s->data[0] = defaults->default_data[0];
    s->data[1] = defaults->default_data[1];
    s->data[2] = defaults->default_data[2];

    Bzzt_Board_Set_Tile(b, x, y, tile);
    if (!Bzzt_Board_Add_Stat(b, s)) // Attempt to add stat to board, if it fails, clean up and return NULL
    {
        Bzzt_Board_Set_Tile(b, x, y, s->under);
        Bzzt_Stat_Destroy(s);
        return NULL;
    }
    return s;
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

    tile.x = x;
    tile.y = y;
    b->tiles[y * b->width + x] = tile;
    return true;
}

bool Bzzt_Board_Is_In_Bounds(Bzzt_Board *b, int x, int y)
{
    return (x >= 0 && y >= 0 && x < b->width && y < b->height);
}

void update_tile_neighbors(Bzzt_Tile tile)
{
    (void)tile;
}

void Bzzt_Board_Move_Tile_To(Bzzt_Board *b, Bzzt_Tile tile, int x, int y)
{
    if (!b || x < 0 || x >= b->width || y < 0 || y >= b->height)
        return;
    int prev_x = tile.x;
    int prev_y = tile.y;
    tile.x = x;
    tile.y = y;
    Bzzt_Board_Set_Tile(b, prev_x, prev_y, empty_tile);
    Bzzt_Board_Set_Tile(b, x, y, tile);
}

void Bzzt_Board_Move_Stat_To(Bzzt_Board *board, Bzzt_Stat *stat, int new_x, int new_y)
{
    if (!board || !stat)
        return;

    stat->prev_x = stat->x;
    stat->prev_y = stat->y;

    Bzzt_Tile stat_tile = Bzzt_Board_Get_Tile(board, stat->x, stat->y);
    Bzzt_Tile new_under = Bzzt_Board_Get_Tile(board, new_x, new_y);

    stat_tile.x = stat->x;
    stat_tile.y = stat->y; // make sure tile pos is up to date

    if (stat_tile.element != ZZT_PLAYER)
        stat_tile.bg = new_under.bg; // Preserve background color except for player

    Bzzt_Board_Move_Tile_To(board, stat_tile, new_x, new_y);

    Bzzt_Board_Set_Tile(board, stat->prev_x, stat->prev_y, stat->under);

    stat->under = new_under;

    stat->x = new_x;
    stat->y = new_y;

    int prev_cell = board_cell_index(board, stat->prev_x, stat->prev_y);
    int new_cell = board_cell_index(board, stat->x, stat->y);
    int stat_idx = Bzzt_Board_Get_Stat_Index(board, stat);
    if (board->stat_index_grid && stat_idx >= 0)
    {
        if (prev_cell >= 0)
            board->stat_index_grid[prev_cell] = -1;
        if (new_cell >= 0)
            board->stat_index_grid[new_cell] = stat_idx;
    }
}

bool Bzzt_Stat_Is_Blocked(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *s, Direction dir)
{
    if (!b || !s)
        return false;

    switch (dir)
    {
    case DIR_UP:
        if (Bzzt_Board_Is_In_Bounds(b, s->x, s->y - 1))
        {
            Bzzt_Tile t = Bzzt_Board_Get_Tile(b, s->x, s->y - 1);
            return !Bzzt_Tile_Is_Walkable(w, t);
        }
        return true;
    case DIR_DOWN:
        if (Bzzt_Board_Is_In_Bounds(b, s->x, s->y + 1))
        {
            Bzzt_Tile t = Bzzt_Board_Get_Tile(b, s->x, s->y + 1);
            return !Bzzt_Tile_Is_Walkable(w, t);
        }
        return true;
    case DIR_LEFT:
        if (Bzzt_Board_Is_In_Bounds(b, s->x - 1, s->y))
        {
            Bzzt_Tile t = Bzzt_Board_Get_Tile(b, s->x - 1, s->y);
            return !Bzzt_Tile_Is_Walkable(w, t);
        }
        return true;
    case DIR_RIGHT:
        if (Bzzt_Board_Is_In_Bounds(b, s->x + 1, s->y))
        {
            Bzzt_Tile t = Bzzt_Board_Get_Tile(b, s->x + 1, s->y);
            return !Bzzt_Tile_Is_Walkable(w, t);
        }
        return true;
    default:
        return true;
    }
}

Bzzt_Stat *Bzzt_Stat_Shoot(Bzzt_Board *b, Bzzt_Stat *shooter, Direction dir)
{
    return Bzzt_Stat_Fire_Projectile(b, shooter, dir, ZZT_BULLET, 0);
}

Bzzt_Stat *Bzzt_Stat_Fire_Projectile(Bzzt_Board *b, Bzzt_Stat *shooter, Direction dir, uint8_t element, uint8_t lifetime_ticks)
{
    if (!b || !shooter || dir == DIR_NONE)
        return NULL;

    const ZZT_Element_Defaults *defaults = zzt_get_element_defaults(element);
    if (!defaults)
        return NULL;

    int projectile_x = 0;
    int projectile_y = 0;
    int step_x = 0;
    int step_y = 0;

    switch (dir)
    {
    case DIR_UP:
        projectile_x = shooter->x;
        projectile_y = shooter->y - 1;
        step_x = 0;
        step_y = -1;
        break;
    case DIR_DOWN:
        projectile_x = shooter->x;
        projectile_y = shooter->y + 1;
        step_x = 0;
        step_y = 1;
        break;
    case DIR_LEFT:
        projectile_x = shooter->x - 1;
        projectile_y = shooter->y;
        step_x = -1;
        step_y = 0;
        break;
    case DIR_RIGHT:
        projectile_x = shooter->x + 1;
        projectile_y = shooter->y;
        step_x = 1;
        step_y = 0;
        break;
    default:
        return NULL;
    }

    if (!Bzzt_Board_Is_In_Bounds(b, projectile_x, projectile_y))
        return NULL;

    Bzzt_Tile target_tile = Bzzt_Board_Get_Tile(b, projectile_x, projectile_y);
    if (!projectile_can_enter_tile(target_tile))
        return NULL;

    Bzzt_Stat *projectile = Bzzt_Board_Spawn_Stat(b, element, projectile_x, projectile_y,
                                                  bzzt_get_color(defaults->default_fg_idx),
                                                  bzzt_get_color(defaults->default_bg_idx));
    if (!projectile)
        return NULL;

    projectile->step_x = step_x;
    projectile->step_y = step_y;
    if (element == ZZT_BULLET)
        projectile->data[0] = Bzzt_Board_Get_Stat_Index(b, shooter);
    else if (element == ZZT_STAR)
        projectile->data[0] = lifetime_ticks;
    return projectile;
}

Bzzt_Board *Bzzt_Board_From_ZZT_Board(ZZTworld *zw)
{
    if (!zw)
        return NULL;

    ZZTblock *block = zztBoardGetBlock(zw);
    Bzzt_Board *bzzt_board = Bzzt_Board_Create((const char *)zztBoardGetTitle(zw), block->width, block->height);

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
