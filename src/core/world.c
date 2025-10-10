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

#define BLINK_RATE_DEFAULT 269 // in ms

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

static void move_stat_to(Bzzt_Board *board, Bzzt_Stat *stat, int new_x, int new_y)
{
    Bzzt_Tile stat_tile = Bzzt_Board_Get_Tile(board, stat->x, stat->y);
    Bzzt_Tile new_under = Bzzt_Board_Get_Tile(board, new_x, new_y);

    Bzzt_Board_Set_Tile(board, stat->x, stat->y, stat->under);

    stat->under = new_under;

    stat->x = new_x;
    stat->y = new_y;

    Bzzt_Board_Set_Tile(board, new_x, new_y, stat_tile);
}

static bool handle_board_edge_move(Bzzt_World *w, int new_x, int new_y)
{
    if (!w)
        return false;

    Bzzt_Board *old_board = w->boards[w->boards_current];
    Bzzt_Stat *old_player = old_board->stats[0];

    uint8_t next_board_idx = 0;

    if (new_x < 0)
        next_board_idx = old_board->board_w;
    else if (new_x >= old_board->width)
        next_board_idx = old_board->board_e;
    else if (new_y < 0)
        next_board_idx = old_board->board_n;
    else if (new_y >= old_board->height)
        next_board_idx = old_board->board_s;

    if (next_board_idx == 0 || next_board_idx > w->boards_count)
    {
        Debug_Log(LOG_WARNING, LOG_WORLD, "Tried to transition to invalid board index.");
        return false;
    }

    Bzzt_Board *new_board = w->boards[next_board_idx];

    int entry_x, entry_y;

    if (new_x < 0)
    {
        entry_x = new_board->width - 1;
        entry_y = old_player->y;
    }
    else if (new_x >= old_board->width)
    {
        entry_x = 0;
        entry_y = old_player->y;
    }
    else if (new_y < 0)
    {
        entry_x = old_player->x;
        entry_y = new_board->height - 1;
    }
    else if (new_y >= old_board->height)
    {
        entry_x = old_player->x;
        entry_y = 0;
    }

    Bzzt_Board_Set_Tile(old_board, old_player->x, old_player->y, old_player->under);
    w->boards_current = next_board_idx;
    Bzzt_Stat *new_player = new_board->stats[0];
    Bzzt_Tile entry_under = Bzzt_Board_Get_Tile(new_board, entry_x, entry_y);
    Bzzt_Board_Set_Tile(new_board, new_player->x, new_player->y, new_player->under);
    new_player->x = entry_x;
    new_player->y = entry_y;
    new_player->under = entry_under;

    Bzzt_Tile player_tile = {0};
    player_tile.element = ZZT_PLAYER;
    player_tile.glyph = 2;
    player_tile.fg = COLOR_WHITE;
    player_tile.bg = COLOR_BLUE;
    player_tile.visible = true;

    Bzzt_Board_Set_Tile(new_board, entry_x, entry_y, player_tile);
    return true;
}

static bool switch_board_to(Bzzt_World *w, int idx, int x, int y)
{
    if (w->boards_count < idx || idx < 0 || w->boards_current == idx)
    {
        Debug_Log(LOG_WARNING, LOG_WORLD, "Tried to transition to invalid board index.");
        return false;
    }
    Bzzt_Board *new_board = w->boards[idx];

    if (x >= new_board->width || x < 0 || y >= new_board->height || y < 0)
    {
        Debug_Log(LOG_WARNING, LOG_WORLD, "Tried to transition to invalid board coordinates.");
        return false;
    }

    Bzzt_Board *old_board = w->boards[w->boards_current];
    Bzzt_Stat *old_player = old_board->stats[0];

    w->boards_current = idx;

    Bzzt_Stat *new_player = new_board->stats[0];

    Bzzt_Board_Set_Tile(old_board, old_player->x, old_player->y, old_player->under);
    Bzzt_Board_Set_Tile(new_board, new_player->x, new_player->y, new_player->under);
    Bzzt_Tile entry_under = Bzzt_Board_Get_Tile(new_board, x, y);

    new_player->x = x;
    new_player->y = y;
    new_player->under = entry_under;

    Bzzt_Tile player_tile = {0};
    player_tile.element = ZZT_PLAYER;
    player_tile.glyph = 2;
    player_tile.fg = COLOR_WHITE;
    player_tile.bg = COLOR_BLUE;
    player_tile.visible = true;

    Bzzt_Board_Set_Tile(new_board, x, y, player_tile);

    if (idx == 0)
        w->on_title = true;
    else
        w->on_title = false;

    return true;
}

static bool handle_passage_touch(Bzzt_World *w, int x, int y)
{
    if (!w)
        return false;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    Bzzt_Stat *passage = Bzzt_Board_Get_Stat_At(current_board, x, y);
    if (!passage)
        return false;

    int target_board_idx = passage->data[2];

    if (target_board_idx <= 0 || target_board_idx >= w->boards_count)
        return false;

    Bzzt_Board *target_board = w->boards[target_board_idx];

    Bzzt_Tile passage_tile = Bzzt_Board_Get_Tile(current_board, passage->x, passage->y);
    Color_Bzzt passage_fg = passage_tile.fg;
    Color_Bzzt passage_bg = passage_tile.bg;

    Bzzt_Stat *matching_passage = NULL;

    int dest_x = target_board->width / 2;  // If no matching passage is found on target board,
    int dest_y = target_board->height / 2; // coords default to center of board

    // Search for linking passage
    for (int i = 0; i < target_board->stat_count; ++i)
    {
        Bzzt_Stat *s = target_board->stats[i];
        Bzzt_Tile t = Bzzt_Board_Get_Tile(target_board, s->x, s->y);
        if (t.element == ZZT_PASSAGE &&
            bzzt_color_equals(t.fg, passage_fg) &&
            bzzt_color_equals(t.bg, passage_bg))
            matching_passage = s;
    }

    if (matching_passage != NULL)
    {
        dest_x = matching_passage->x;
        dest_y = matching_passage->y;
    }

    switch_board_to(w, target_board_idx, dest_x, dest_y);
    Bzzt_World_Set_Pause(w, true);
    return true;
}

static bool handle_player_touch(Bzzt_World *w, Bzzt_Tile tile, int x, int y)
{
    if (!w)
        return false;

    Bzzt_Board *board = w->boards[w->boards_current];
    Bzzt_Stat *player = board->stats[0];
    Bzzt_Tile player_tile = Bzzt_Board_Get_Tile(board, player->x, player->y);

    Interaction_Type type = Bzzt_Tile_Get_Interaction_Type(tile);
    const char *type_name = Bzzt_Tile_Get_Type_Name(tile);
    Debug_Printf(LOG_LEVEL_DEBUG, "Player touched %s at (%d, %d).", type_name, x, y);

    // Handle solid, interactable tiles here
    switch (tile.element)
    {
    case ZZT_PASSAGE:
        return handle_passage_touch(w, x, y);
    default:
        break;
    }

    return true;
}

static void update_stat_direction(Bzzt_Stat *stat, int dx, int dy)
{
    if (dx != 0)
        stat->step_x = (dx > 0) ? 1 : -1;
    if (dy != 0)
        stat->step_y = (dy > 0) ? 1 : -1;
}

static bool handle_item_pickup(Bzzt_World *w, Bzzt_Board *b, int x, int y, Bzzt_Tile item)
{
    Bzzt_Tile empty = {0};
    Bzzt_Stat *player = b->stats[0];
    player->under = empty;
    switch (item.element)
    {
    case ZZT_AMMO:
        w->ammo += 5;
        return true;
    case ZZT_GEM:
        w->gems++;
        w->score += 10;
        return true;
    case ZZT_TORCH:
        w->torches++;
        return true;
    case ZZT_ENERGIZER:
    case ZZT_KEY:
    case ZZT_SCROLL:
        return true;
    case ZZT_FOREST:
        Debug_Printf(LOG_WORLD, "Moved through the forest.");
        return true;
    default:
        return false;
    }
}

static bool do_player_move(Bzzt_World *w, int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return false;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    if (!current_board || current_board->stat_count == 0)
        return false;

    Bzzt_Stat *player = current_board->stats[0];
    if (!player)
        return false;

    int new_x = player->x + dx;
    int new_y = player->y + dy;

    // If trying to move past board bounds, try moving to adjacent board
    if (new_x < 0 || new_x >= current_board->width || new_y < 0 || new_y >= current_board->height)
    {
        // Quirk - if paused, ZZT stays paused during board edge transition
        return handle_board_edge_move(w, new_x, new_y);
    }

    Bzzt_Tile target = Bzzt_Board_Get_Tile(current_board, new_x, new_y);

    // If won't collide with anything
    if (Bzzt_Tile_Is_Walkable(target))
    {
        move_stat_to(current_board, player, new_x, new_y);
        // If moved on to item pickup
        handle_item_pickup(w, current_board, new_x, new_y, target);
    }

    // Unpause is player managed to move 1 tile
    if (w->paused)
        Bzzt_World_Set_Pause(w, false);

    return handle_player_touch(w, target, new_x, new_y);
}

static void update_player(Bzzt_World *w, InputState *in)
{
    if (!in->anyDirPressed || in->delayLock)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];

    if (!current_board || current_board->stat_count == 0)
        return;

    Bzzt_Stat *player = current_board->stats[0];

    if (!player)
        return;

    if (do_player_move(w, in->dx, in->dy))
        update_stat_direction(player, in->dx, in->dy);
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
    w->on_title = false;

    w->blink_delay_rate = BLINK_RATE_DEFAULT;
    w->blink_timer = 0.0;
    w->allow_blink = true; // blink on by default
    w->blink_state = false;

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

    if (w->allow_blink)
    {
        double delta_time = GetFrameTime() * 1000.0;
        w->blink_timer += delta_time;
        if (w->blink_timer >= w->blink_delay_rate)
        {
            w->blink_state = !w->blink_state;
            w->blink_timer = 0.0;
        }
    }
    if (w->boards_current != 0)
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

// Exposed version of this
bool Bzzt_World_Switch_Board_To(Bzzt_World *w, int board_idx, int x, int y)
{
    if (!w || w->boards_current == board_idx || w->boards_count < board_idx)
        return false;

    return switch_board_to(w, board_idx, x, y);
}

void Bzzt_World_Set_Pause(Bzzt_World *w, bool pause)
{
    if (!w)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    Bzzt_Stat *player = current_board->stats[0];
    Bzzt_Tile tile = Bzzt_Board_Get_Tile(current_board, player->x, player->y);

    if (pause)
        tile.blink = true;
    else
        tile.blink = false;

    Bzzt_Board_Set_Tile(current_board, player->x, player->y, tile);

    w->paused = pause;
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
    }

    int boardCount = zztWorldGetBoardcount(zw);

    for (int i = 0; i < boardCount; ++i)
    {
        zztBoardSelect(zw, i);
        Bzzt_Board *b = Bzzt_Board_From_ZZT_Board(zw);
        if (!b)
            return NULL;
        b->idx = i;
        Bzzt_World_Add_Board(bw, b);
    }

    bw->boards_current = 0;
    bw->start_board_idx = zztWorldGetStartboard(zw);
    bw->start_board = bw->boards[bw->start_board_idx];

    bw->ammo = 0;
    bw->gems = 0;
    bw->energizer_cycles = 0;
    bw->health = 100;
    // keys
    bw->score = 0;
    bw->torch_cycles = 0;
    bw->torches = 0;

    // verify player exists
    if (bw->start_board->stat_count > 0)
    {
        Bzzt_Stat *player_stat = bw->start_board->stats[0];
        Bzzt_Tile player_tile = Bzzt_Board_Get_Tile(bw->start_board, player_stat->x, player_stat->y);
        if (player_tile.element != ZZT_PLAYER)
        {
            Debug_Log(LOG_LEVEL_WARN, LOG_WORLD, "Stat[0] is not a player.");
        }
    }
    else
    {
        Debug_Log(LOG_LEVEL_WARN, LOG_WORLD, "Found no stats on start board.");
    }

    bw->on_title = true;

    zztWorldFree(zw);

    return bw;
}
