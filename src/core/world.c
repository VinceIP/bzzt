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
#include "timing.h"

#define BLINK_RATE_DEFAULT 269   // in ms
#define TICK_DURATION_DEFAULT 80 // in ms
#define STICKY_INPUT_DEFAULT true

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

static void update_stat_direction(Bzzt_Stat *stat, int dx, int dy)
{
    if (dx != 0)
        stat->step_x = (dx > 0) ? 1 : -1;
    if (dy != 0)
        stat->step_y = (dy > 0) ? 1 : -1;
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

    w->timer = malloc(sizeof(Bzzt_Timer));
    w->timer->accumulator_ms = 0;
    w->timer->current_stat_index = 0;
    w->timer->current_tick = 1;
    w->timer->paused = false;
    w->timer->tick_duration_ms = TICK_DURATION_DEFAULT;

    w->enable_sticky_input = STICKY_INPUT_DEFAULT;
    w->current_input = NULL;
    w->has_queued_input = false;
    w->queued_dx = 0;
    w->queued_dy = 0;

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
    if (w->timer)
        free(w->timer);
    free(w);
}

void Bzzt_World_Update(Bzzt_World *w, InputState *in)
{
    if (!w || !in)
        return;
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

    // Queue inputs
    if (in->anyDirPressed && w->enable_sticky_input)
    {
        w->queued_dx = in->dx;
        w->queued_dy = in->dy;
        w->has_queued_input = true;
    }

    w->current_input = in;

    double frame_ms = GetFrameTime() * 1000.0;
    Bzzt_Timer_Run_Frame(w, frame_ms);
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

void Bzzt_World_Inc_Score(Bzzt_World *w, int amount)
{
    if (!w)
        return;
    w->score += amount;
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
    bw->score = 0;
    bw->torch_cycles = 0;
    bw->torches = 0;

    for (int i = 0; i < 7; ++i)
        bw->keys[i] = 0;

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
