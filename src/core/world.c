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
#include "ui.h"

#define BLINK_RATE_DEFAULT 269   // in ms
#define TICK_DURATION_DEFAULT 109.89 // Original ZZT runs at roughly 9.1 logic cycles per second.

static void initialize_loaded_world_state(Bzzt_World *bw)
{
    if (!bw)
        return;

    bw->boards_current = 0;
    bw->ammo = 0;
    bw->gems = 0;
    bw->energizer_cycles = 0;
    bw->health = 100;
    bw->score = 0;
    bw->torch_cycles = 0;
    bw->torches = 0;
    bw->player_hurt_flash_ticks = 0;

    for (int i = 0; i < 7; ++i)
        bw->keys[i] = 0;
}

static int extra_player_clone_count(Bzzt_Board *board)
{
    int count = 0;

    if (!board)
        return 0;

    for (int i = 1; i < board->stat_count; ++i)
    {
        Bzzt_Stat *stat = board->stats[i];
        if (!stat || !Bzzt_Board_Is_In_Bounds(board, stat->x, stat->y))
            continue;

        if (Bzzt_Board_Get_Tile(board, stat->x, stat->y).element == ZZT_PLAYER)
            count++;
    }

    return count;
}

static Bzzt_Tile make_avatar_tile(void)
{
    Bzzt_Tile avatar = {0};

    avatar.element = ZZT_PLAYER;
    avatar.glyph = zzt_type_to_cp437(avatar.element, 0x1F);
    avatar.fg = COLOR_WHITE;
    avatar.bg = COLOR_BLUE;
    avatar.visible = true;

    return avatar;
}

static void set_board_avatar_tile(Bzzt_Board *board, Bzzt_Stat *avatar_stat)
{
    if (!board || !avatar_stat)
        return;

    Bzzt_Tile avatar = make_avatar_tile();
    Bzzt_Board_Set_Tile(board, avatar_stat->x, avatar_stat->y, avatar);
}

static void hide_title_monitor(Bzzt_World *w, Bzzt_Board *board, Bzzt_Stat *monitor, bool restore_under)
{
    if (!w || !board || !monitor)
        return;

    if (restore_under &&
        Bzzt_Board_Is_In_Bounds(board, w->title_monitor_x, w->title_monitor_y))
    {
        Bzzt_Board_Set_Tile(board, w->title_monitor_x, w->title_monitor_y, monitor->under);
    }

    monitor->x = -1;
    monitor->y = -1;
    monitor->prev_x = -1;
    monitor->prev_y = -1;
    Bzzt_Board_Rebuild_Stat_Index(board);
}

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
    if (!w || idx < 0 || idx >= w->boards_count || w->boards_current == idx)
    {
        Debug_Log(LOG_LEVEL_WARN, LOG_WORLD, "Tried to transition to invalid board index.");
        return false;
    }

    Debug_Log(LOG_LEVEL_DEBUG, LOG_WORLD, "Switching to board %d at %d, %d.", idx, x, y);

    Bzzt_Board *new_board = w->boards[idx];

    Bzzt_Board *old_board = w->boards[w->boards_current];
    Bzzt_Stat *old_player = old_board->stats[0];

    w->boards_current = idx;

    Bzzt_Stat *new_player = new_board->stats[0];

    w->on_title = (idx == 0);
    if (Bzzt_Board_Is_In_Bounds(old_board, old_player->x, old_player->y))
        Bzzt_Board_Set_Tile(old_board, old_player->x, old_player->y, old_player->under);
    Bzzt_Board_Rebuild_Stat_Index(old_board);

    if (w->on_title)
    {
        hide_title_monitor(w, new_board, new_player, false);
        return true;
    }

    if (Bzzt_Board_Is_In_Bounds(new_board, new_player->x, new_player->y))
        Bzzt_Board_Set_Tile(new_board, new_player->x, new_player->y, new_player->under);

    Bzzt_Tile entry_under = Bzzt_Board_Get_Tile(new_board, x, y);
    new_player->x = x;
    new_player->y = y;
    new_player->under = entry_under;
    set_board_avatar_tile(new_board, new_player);
    Bzzt_Board_Rebuild_Stat_Index(new_board);

    return true;
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
    w->loaded = true;
    w->on_title = false;
    w->paused = false;
    w->title_monitor_x = -1;
    w->title_monitor_y = -1;

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

    w->last_frame_time_ms = GetTime() * 1000.0;

#if BZZT_ENABLE_INTERPOLATION
    w->interpolation_enabled = true;
#else
    w->interpolation_enabled = false;
#endif

    w->game_speed = 4;
    w->player_hurt_flash_ticks = 0;
    w->energizer_cycles = 0;

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
    w->loaded = false;
    free(w->boards);
    if (w->timer)
        free(w->timer);
    free(w);
}

void Bzzt_World_Update(Engine *e, Bzzt_World *w, InputState *in)
{
    if (!w || !in)
        return;

    double current_time_ms = GetTime() * 1000.0;
    double delta_time = current_time_ms - w->last_frame_time_ms;
    w->last_frame_time_ms = current_time_ms;
    if (delta_time > 250.0)
        delta_time = 250.0;
    if (delta_time < 0.0)
        delta_time = 0.0;

    if (w->allow_blink)
    {
        w->blink_timer += delta_time;
        if (w->blink_timer >= w->blink_delay_rate)
        {
            w->blink_state = !w->blink_state;
            w->blink_timer = 0.0;
        }
    }

    w->current_input = in;

    if (w->paused && !w->on_title)
    {
        bool wants_to_resume = in->SPACE_pressed ||
                               in->input_buffer_count > 0 ||
                               in->arrow_stack_count > 0;
        if (!wants_to_resume)
            return;

        Bzzt_World_Set_Pause(w, false);
    }

    Bzzt_Timer_Run_Frame(e, w, delta_time);
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
// TODO: handle landing on a forest tile (clear it)
bool Bzzt_World_Switch_Board_To(Bzzt_World *w, int board_idx, int x, int y)
{
    if (!w || board_idx < 0 || board_idx >= w->boards_count || w->boards_current == board_idx)
        return false;

    return switch_board_to(w, board_idx, x, y);
}

void Bzzt_World_Set_Pause(Bzzt_World *w, bool pause)
{
    if (!w)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    Bzzt_Stat *player = current_board ? current_board->stats[0] : NULL;

    if (!w->on_title && current_board && player &&
        Bzzt_Board_Is_In_Bounds(current_board, player->x, player->y))
    {
        Bzzt_Tile tile = Bzzt_Board_Get_Tile(current_board, player->x, player->y);
        tile.blink = pause;
        Bzzt_Board_Set_Tile(current_board, player->x, player->y, tile);
    }

    w->paused = pause;
    if (w->timer)
        w->timer->paused = pause;
}

void Bzzt_World_Inc_Score(Bzzt_World *w, int amount)
{
    if (!w)
        return;
    w->score += amount;
}

int Bzzt_World_Normalize_Game_Speed(int game_speed)
{
    if (game_speed < 0)
        return 0;
    if (game_speed > 8)
        return 8;
    return game_speed;
}

int Bzzt_World_Message_Ticks(Bzzt_World *w)
{
    int game_speed = Bzzt_World_Normalize_Game_Speed(w ? w->game_speed : 4);
    int ticks = 200 / ((game_speed * 2) + 1);
    return ticks > 0 ? ticks : 1;
}

bool Bzzt_World_Is_Energized(Bzzt_World *w)
{
    return w && w->energizer_cycles > 0;
}

void Bzzt_World_Begin_Tick(Bzzt_World *w)
{
    if (!w)
        return;

    if (w->player_hurt_flash_ticks > 0)
        w->player_hurt_flash_ticks--;
}

void Bzzt_World_Advance_Status_Effects(Bzzt_World *w)
{
    if (!w || !Bzzt_World_Is_Energized(w))
        return;

    Bzzt_Board *board = w->boards[w->boards_current];
    int drain = 1 + extra_player_clone_count(board);
    w->energizer_cycles -= drain;
    if (w->energizer_cycles < 0)
        w->energizer_cycles = 0;
}

bool Bzzt_World_Damage_Player(UI *ui, Bzzt_World *w, int amount, Bzzt_DamageSource source)
{
    if (!w)
        return false;

    if (source != BZZT_DAMAGE_SOURCE_BOMB &&
        source != BZZT_DAMAGE_SOURCE_SCRIPT &&
        source != BZZT_DAMAGE_SOURCE_ENDGAME &&
        Bzzt_World_Is_Energized(w))
        return false;

    if (amount <= 0)
        amount = 10;

    w->health -= amount;
    if (w->health < 0)
        w->health = 0;
    w->player_hurt_flash_ticks = 1;
    UI_Flash_Message(ui, w, ZZT_MSG_OUCH);
    return true;
}

Bzzt_Tile Bzzt_World_Get_Player_Render_Tile(Bzzt_World *w, Bzzt_Tile tile)
{
    static const Color_Bzzt energizer_bg_cycle[] = {
        COLOR_BLUE,
        COLOR_GREEN,
        COLOR_CYAN,
        COLOR_RED,
        COLOR_MAGENTA,
        COLOR_BROWN,
        COLOR_DARK_GRAY
    };

    if (!w || tile.element != ZZT_PLAYER)
        return tile;

    if (w->player_hurt_flash_ticks > 0)
    {
        tile.fg = COLOR_WHITE;
        tile.bg = COLOR_DARK_GRAY;
        return tile;
    }

    if (Bzzt_World_Is_Energized(w) && w->timer)
    {
        int phase = w->timer->current_tick % (int)(sizeof(energizer_bg_cycle) / sizeof(energizer_bg_cycle[0]));
        tile.bg = energizer_bg_cycle[phase];
    }

    return tile;
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

    if (bw->boards_count > 0 && bw->boards[0] && bw->boards[0]->stat_count > 0)
    {
        bw->title_monitor_x = bw->boards[0]->stats[0]->x;
        bw->title_monitor_y = bw->boards[0]->stats[0]->y;
        hide_title_monitor(bw, bw->boards[0], bw->boards[0]->stats[0], true);
    }

    bw->on_title = true;

    zztWorldFree(zw);

    return bw;
}

Bzzt_World *Bzzt_World_From_ZZT_Stream(FILE *fp, const char *display_name)
{
    if (!fp)
    {
        Debug_Printf(LOG_ENGINE, "Invalid ZZT stream.");
        return NULL;
    }

    ZZTworld *zw = zztWorldRead(fp);
    if (!zw)
    {
        Debug_Printf(LOG_ENGINE, "Error loading ZZT world from stream.");
        return NULL;
    }

    Bzzt_World *bw = Bzzt_World_Create((char *)zztWorldGetTitle(zw));
    strncpy(bw->file_path,
            display_name ? display_name : "<zip world>",
            sizeof(bw->file_path) - 1);
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

    bw->start_board_idx = zztWorldGetStartboard(zw);
    bw->start_board = bw->boards[bw->start_board_idx];
    initialize_loaded_world_state(bw);

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

    if (bw->boards_count > 0 && bw->boards[0] && bw->boards[0]->stat_count > 0)
    {
        bw->title_monitor_x = bw->boards[0]->stats[0]->x;
        bw->title_monitor_y = bw->boards[0]->stats[0]->y;
        hide_title_monitor(bw, bw->boards[0], bw->boards[0]->stats[0], true);
    }

    bw->on_title = true;

    zztWorldFree(zw);

    return bw;
}

void Bzzt_World_Toggle_Interpolation(Bzzt_World *w)
{
    if (!w)
        return;

#if BZZT_ENABLE_INTERPOLATION
    w->interpolation_enabled = !w->interpolation_enabled;
    Debug_Printf(LOG_ENGINE, "Interpolation %s",
                 w->interpolation_enabled ? "ENABLED" : "DISABLED");
#else
    Debug_Printf(LOG_ENGINE, "Interpolation disabled at compile time");
#endif
}
