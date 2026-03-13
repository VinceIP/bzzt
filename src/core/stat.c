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
#include "timing.h"
#include "input.h"
#include "color.h"
#include "zzt_element_defaults.h"
#include "ui.h"

// Note - i hate this entire file
// tbd - stop passing in boards, worlds, ui, etc to all these functions. instead pass in one engine context. maybe?

static const Bzzt_Tile empty_tile = {0};

static void clear_forest(UI *ui, Bzzt_Board *b, int x, int y);

static bool world_is_on_title_screen(Bzzt_World *w)
{
    return w && w->on_title;
}

// Maps any color (light or dark variant) to a key index 0-6.
// Palette indices 1-7 and 9-15 pair up via (idx & 7): e.g. BZ_GREEN(2) and
// BZ_LIGHT_GREEN(10) both give 2 & 7 = 2 -> ZZT_KEY_GREEN. Returns -1 if not
// a valid key color (black, dark gray, transparent).
static int get_key_index_from_color(Color_Bzzt color)
{
    int pal = bzzt_color_to_index(color);
    if (pal < 0)
        return -1;
    int base = pal & 7; // normalize: collapses light/dark pairs to 1-7
    if (base < 1 || base > 7)
        return -1;
    return base - 1; // 0=blue, 1=green, 2=cyan, 3=red, 4=purple, 5=yellow, 6=white
}

static const char *get_key_color_name(int key_index)
{
    const char *names[] = {"Blue", "Green", "Cyan", "Red", "Purple",
                           "Yellow", "White"};
    return (key_index >= 0 && key_index < 7) ? names[key_index] : "Unknown";
}

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
    if (!w || !stat || !w->timer)
        return false;

    if (stat->cycle == 0)
        return false;

    int current_tick = w->timer->current_tick;
    return (current_tick % stat->cycle) == (stat_idx % stat->cycle);
}

static void handle_bullet_collision(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *bullet, int collision_x, int collision_y)
{
    if (!b || !bullet)
        return;

    Bzzt_Tile collision_tile = Bzzt_Board_Get_Tile(b, collision_x, collision_y);
    int bullet_idx = Bzzt_Board_Get_Stat_Index(b, bullet);

    const Bzzt_Tile empty = {0};

    bool is_player_bullet = (bullet->data[0] == 0); // Player bullets have data[0] == 0

    switch (collision_tile.element)
    {
    case ZZT_BREAKABLE:
        Bzzt_Board_Set_Tile(b, collision_x, collision_y, empty); // Kill breakable and bullet
        Bzzt_Board_Stat_Die(b, bullet);
        break; // pun
    case ZZT_RICOCHET:
        bullet->step_x = -bullet->step_x;
        bullet->step_y = -bullet->step_y;
        break;
    case ZZT_OBJECT:
        // tbd
        break;
    case ZZT_BULLET: // bullets destroy each other
        Bzzt_Stat *other_bullet = Bzzt_Board_Get_Stat_At(b, collision_x, collision_y);
        if (other_bullet)
            Bzzt_Board_Stat_Die(b, other_bullet);
        Bzzt_Board_Stat_Die(b, bullet);
        break;
    case ZZT_BEAR:
        Bzzt_Stat *bear = Bzzt_Board_Get_Stat_At(b, collision_x, collision_y);
        if (bear && is_player_bullet)
        {
            Bzzt_Board_Stat_Die(b, bear);
            Bzzt_Board_Stat_Die(b, bullet);
            Bzzt_World_Inc_Score(w, 1);
        }
        break;
    case ZZT_CENTHEAD:
        break;
    case ZZT_CENTBODY:
        break;
    case ZZT_LION:
        Bzzt_Stat *lion = Bzzt_Board_Get_Stat_At(b, collision_x, collision_y);
        if (lion && is_player_bullet)
        {
            Bzzt_Board_Stat_Die(b, lion);
            Bzzt_Board_Stat_Die(b, bullet);
            Bzzt_World_Inc_Score(w, 1);
        }
        break;
    case ZZT_RUFFIAN:
        Bzzt_Stat *ruffian = Bzzt_Board_Get_Stat_At(b, collision_x, collision_y);
        if (ruffian && is_player_bullet)
        {
            Bzzt_Board_Stat_Die(b, ruffian);
            Bzzt_Board_Stat_Die(b, bullet);
            Bzzt_World_Inc_Score(w, 2);
        }
        break;
    case ZZT_SHARK:
        break;
    case ZZT_TIGER:
        Bzzt_Stat *tiger = Bzzt_Board_Get_Stat_At(b, collision_x, collision_y);
        if (tiger && is_player_bullet)
        {
            Bzzt_Board_Stat_Die(b, tiger);
            Bzzt_Board_Stat_Die(b, bullet);
            Bzzt_World_Inc_Score(w, 2);
        }
        break;
    case ZZT_PLAYER:
        Bzzt_Board_Stat_Die(b, bullet);
        UI_Flash_Message(ui, ZZT_MSG_OUCH);
        // tbd player collision
        break;
    default: // Hitting any other element kills the bullet
        Bzzt_Board_Stat_Die(b, bullet);
        break;
    }
}

// tbd: replace with emulating zzt's board edge element
static bool handle_board_edge_move(Bzzt_World *w, UI *ui, int new_x, int new_y)
{
    if (!w)
        return false;

    Bzzt_Board *old_board = w->boards[w->boards_current];
    Bzzt_Board *new_board;
    Bzzt_Stat *old_player = old_board->stats[0];

    uint8_t next_board_idx = 0;
    int entry_x, entry_y;

    if (new_x < 0)
    {
        next_board_idx = old_board->board_w;
        new_board = w->boards[next_board_idx];
        entry_x = new_board->width - 1;
        entry_y = old_player->y;
    }
    else if (new_x >= old_board->width)
    {
        next_board_idx = old_board->board_e;
        new_board = w->boards[next_board_idx];
        entry_x = 0;
        entry_y = old_player->y;
    }
    else if (new_y < 0)
    {
        next_board_idx = old_board->board_n;
        new_board = w->boards[next_board_idx];
        entry_x = old_player->x;
        entry_y = new_board->height - 1;
    }
    else if (new_y >= old_board->height)
    {
        next_board_idx = old_board->board_s;
        new_board = w->boards[next_board_idx];
        entry_x = old_player->x;
        entry_y = 0;
    }

    if (next_board_idx <= 0 || next_board_idx >= w->boards_count)
        return false; // board idx invalid?

    Bzzt_Board_Set_Tile(old_board, old_player->x, old_player->y, old_player->under);

    return Bzzt_World_Switch_Board_To(w, next_board_idx, entry_x, entry_y);
}

static void clear_forest(UI *ui, Bzzt_Board *b, int x, int y)
{
    Bzzt_Board_Set_Tile(b, x, y, (Bzzt_Tile){0});
    UI_Flash_Message(ui, ZZT_MSG_TOUCH_FOREST);
}

static uint8_t handle_passage_touch(Bzzt_World *w, int x, int y)
{
    if (!w)
        return 0;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    Bzzt_Stat *passage = Bzzt_Board_Get_Stat_At(current_board, x, y);
    if (!passage)
        return 0;

    int target_board_idx = passage->data[2];

    if (target_board_idx <= 0 || target_board_idx >= w->boards_count)
        return 0;

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

    if (Bzzt_World_Switch_Board_To(w, target_board_idx, dest_x, dest_y))
    {
        puts("doing pause");
        Bzzt_World_Set_Pause(w, true);
    }
    return ZZT_PASSAGE;
}

static bool player_has_key(Bzzt_World *w, Bzzt_Tile tile)
{
    Color_Bzzt key_color = tile.fg;
    int key_idx = get_key_index_from_color(key_color);
    Debug_Log(LOG_LEVEL_DEBUG, LOG_ENGINE, "key idx: %d", key_idx);
    if (key_idx < 0 || key_idx >= 7)
        return true; // invalid key index
    if (w->keys[key_idx] != 0)
        return true; // Already have key

    return false;
}

void zzt_touch_enemy(UI *ui, Bzzt_World *w, Bzzt_Board *b, int x, int y)
{
    Bzzt_Stat *enemy = Bzzt_Board_Get_Stat_At(b, x, y);
    if (enemy)
    {
        Bzzt_Board_Stat_Die(b, enemy);
        UI_Flash_Message(ui, ZZT_MSG_OUCH);
        w->health -= 10;
    }
}

void zzt_touch_ammo(UI *ui, Bzzt_World *w, Bzzt_Board *b, int x, int y)
{
    w->ammo += 5;
    Bzzt_Board_Set_Tile(b, x, y, empty_tile);
    UI_Flash_Message(ui, ZZT_MSG_AMMO_GET);
}

void zzt_touch_gem(UI *ui, Bzzt_World *w, Bzzt_Board *b, int x, int y)
{
    w->gems++;
    w->score += 10;
    Bzzt_Board_Set_Tile(b, x, y, empty_tile);
    UI_Flash_Message(ui, ZZT_MSG_GEM_GET);
}

void zzt_touch_torch(UI *ui, Bzzt_World *w, Bzzt_Board *b, int x, int y)
{
    w->torches++;
    Bzzt_Board_Set_Tile(b, x, y, empty_tile);
    UI_Flash_Message(ui, ZZT_MSG_TORCH_GET);
}

// tbd
void zzt_touch_energizer(UI *ui, Bzzt_Board *b, int x, int y)
{
    Bzzt_Board_Set_Tile(b, x, y, empty_tile);
    UI_Flash_Message(ui, ZZT_MSG_ENERGIZER_ACTIVATED);
}

void zzt_touch_key(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Tile t, int x, int y)
{
    int key_idx = get_key_index_from_color(t.fg);
    if (key_idx < 0 || key_idx >= 7)
        return;
    const char *color_name = get_key_color_name(key_idx);
    if (w->keys[key_idx])
    {
        UI_Flash_Message(ui, ZZT_MSG_KEY_ALREADY_HAVE, color_name);
        return;
    }
    w->keys[key_idx] = 1;
    Bzzt_Board_Set_Tile(b, x, y, empty_tile);
    UI_Flash_Message(ui, ZZT_MSG_KEY_GET, color_name);
}

// returns the type of thing the player touched
static uint8_t handle_player_touch(UI *ui, Bzzt_World *w, Bzzt_Tile tile, int x, int y, Direction direction)
{
    if (!w)
        return false;

    Bzzt_Board *b = w->boards[w->boards_current];

    const char *type_name = Bzzt_Tile_Get_Type_Name(tile);
    Debug_Printf(LOG_LEVEL_DEBUG, "Player touched %s at (%d, %d).", type_name, x, y);

    switch (tile.element)
    {
    case ZZT_FOREST:
        clear_forest(ui, b, x, y);
        break;
    case ZZT_BEAR:
    case ZZT_LION:
    case ZZT_TIGER:
    case ZZT_RUFFIAN:
    case ZZT_CENTHEAD:
    case ZZT_CENTBODY:
        zzt_touch_enemy(ui, w, b, x, y);
        break;
    case ZZT_AMMO:
        zzt_touch_ammo(ui, w, b, x, y);
        break;
    case ZZT_GEM:
        zzt_touch_gem(ui, w, b, x, y);
        break;
    case ZZT_TORCH:
        zzt_touch_torch(ui, w, b, x, y);
        break;
    case ZZT_ENERGIZER:
        zzt_touch_energizer(ui, b, x, y);
        break;
    case ZZT_KEY:
        zzt_touch_key(ui, w, b, tile, x, y);
        break;
    case ZZT_SCROLL:
        break;
    case ZZT_PASSAGE:
        return handle_passage_touch(w, x, y);
    case ZZT_INVISIBLE:
        if (!tile.visible)
        {
            tile.visible = true;
            Bzzt_Board_Set_Tile(b, x, y, tile);
            UI_Flash_Message(ui, ZZT_MSG_TOUCH_INVISIBLE_WALL);
        }
        break;
    case ZZT_WATER:
        UI_Flash_Message(ui, ZZT_MSG_TOUCH_WATER);
        break;
    case ZZT_DOOR:
        int key_idx = get_key_index_from_color(tile.bg);
        if (key_idx < 0 || key_idx >= 7)
            return false;
        const char *door_color_name = get_key_color_name(key_idx);
        if (w->keys[key_idx] == 0)
        {
            UI_Flash_Message(ui, ZZT_MSG_DOOR_LOCKED, door_color_name);
            return false;
        }
        w->keys[key_idx] = 0;
        Bzzt_Board_Set_Tile(b, x, y, empty_tile);
        UI_Flash_Message(ui, ZZT_MSG_DOOR_OPEN, door_color_name);
        break;
    case ZZT_BOMB:
    {
        Bzzt_Stat *bomb = Bzzt_Board_Get_Stat_At(b, x, y);
        if (bomb && bomb->data[0] == 0)
        {
            // Inactive bomb — activate it, don't push
            bomb->data[0] = 9;
            Bzzt_Tile bomb_tile = Bzzt_Board_Get_Tile(b, x, y);
            bomb_tile.glyph = '9';
            Bzzt_Board_Set_Tile(b, x, y, bomb_tile);
            UI_Flash_Message(ui, ZZT_MSG_BOMB_ACTIVATED);
        }
        else
        {
            // Active bomb — push it
            push_tile(b, direction, tile);
        }
        break;
    }
    case ZZT_BOULDER:
    case ZZT_EWSLIDER:
    case ZZT_NSSLIDER:
        push_tile(b, direction, tile);
        break;
    default:
        break;
        return tile.element;
    }
    return tile.element;
}

static void handle_player_move(UI *ui, Bzzt_World *w)
{
    if (!w || !w->current_input)
        return;

    if (world_is_on_title_screen(w))
        return;

    InputState *in = w->current_input;

    ArrowKey buffered_key = Input_Pop_Buffered_Direction(in);
    ArrowKey priority_key = buffered_key != ARROW_NONE ? buffered_key : Input_Get_Priority_Direction(in);

    if (priority_key == ARROW_NONE || in->SHIFT_held)
        return; // No movement if no key or shift held

    int dx, dy;
    Input_Get_Direction(priority_key, &dx, &dy);

    if (dx == 0 && dy == 0)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    if (!current_board)
        return;

    Bzzt_Stat *stat = current_board->stats[0];
    if (!stat)
        return;

    int new_x = stat->x + dx;
    int new_y = stat->y + dy;

    if (!Bzzt_Board_Is_In_Bounds(current_board, new_x, new_y))
    {
        handle_board_edge_move(w, ui, new_x, new_y);
        return;
    }

    Direction move_dir = (dx > 0) ? DIR_RIGHT : (dx < 0) ? DIR_LEFT
                                            : (dy > 0)   ? DIR_DOWN
                                                         : DIR_UP;

    Bzzt_Tile target_tile = Bzzt_Board_Get_Tile(current_board, new_x, new_y);
    uint8_t element_type_touched = handle_player_touch(ui, w, target_tile, new_x, new_y, move_dir);

    // Re-fetch after touch: a successful boulder push leaves an empty tile here
    target_tile = Bzzt_Board_Get_Tile(current_board, new_x, new_y);
    if (Bzzt_Tile_Is_Walkable(w, target_tile))
        Bzzt_Board_Move_Stat_To(current_board, stat, new_x, new_y);

    if (dx != 0)
        stat->step_x = (dx > 0) ? 1 : -1;
    if (dy != 0)
        stat->step_y = (dy > 0) ? 1 : -1;

    if (w->paused && element_type_touched != ZZT_PASSAGE)
        Bzzt_World_Set_Pause(w, false);
}

static void handle_player_shoot(UI *ui, Bzzt_World *w, Bzzt_Stat *player_stat)
{
    if (!w || !player_stat)
        return;

    if (world_is_on_title_screen(w))
        return;

    InputState *in = w->current_input;
    if (!in)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    if (!current_board)
        return;

    bool wants_to_shoot = false;
    int shoot_dx = 0, shoot_dy = 0;

    if (in->SPACE_pressed)
    {
        wants_to_shoot = true;
        shoot_dx = player_stat->step_x;
        shoot_dy = player_stat->step_y;
    }

    else if (in->SHIFT_held && in->arrow_stack_count > 0)
    {
        wants_to_shoot = true;
        ArrowKey shoot_key = Input_Get_Priority_Direction(in);
        Input_Get_Direction(shoot_key, &shoot_dx, &shoot_dy);
    }

    if (!wants_to_shoot)
        return;

    if (shoot_dx == 0 && shoot_dy == 0)
        return; // Can't shoot with no direction

    if (current_board->max_shots == 0)
    {
        UI_Flash_Message(ui, ZZT_MSG_SHOT_FORBIDDEN);
        return;
    }

    if (w->ammo <= 0)
    {
        UI_Flash_Message(ui, ZZT_MSG_SHOT_EMPTY);
        return;
    }

    if (current_board->max_shots > 0)
    {
        int bullet_count = Bzzt_Board_Get_Bullet_Count(current_board);
        if (bullet_count >= current_board->max_shots)
            return; // At max shots
    }

    Bzzt_Stat_Shoot(current_board, player_stat,
                    (shoot_dx > 0) ? DIR_RIGHT : (shoot_dx < 0) ? DIR_LEFT
                                             : (shoot_dy > 0)   ? DIR_DOWN
                                             : (shoot_dy < 0)   ? DIR_UP
                                                                : DIR_NONE);

    w->ammo--;
}

// Elliptical blast: uses bzzt_in_radius() from bzzt.h

static bool bomb_element_is_destructible(uint8_t elem)
{
    switch (elem)
    {
    case ZZT_BREAKABLE:
    case ZZT_BEAR:
    case ZZT_RUFFIAN:
    case ZZT_OBJECT:
    case ZZT_SLIME:
    case ZZT_SHARK:
    case ZZT_SPINNINGGUN:
    case ZZT_LION:
    case ZZT_TIGER:
    case ZZT_CENTHEAD:
    case ZZT_CENTBODY:
    case ZZT_BULLET:
    case ZZT_STAR:
        return true;
    default:
        return false;
    }
}

static void spawn_bomb_blast_tile(Bzzt_Board *b, int x, int y)
{
    const ZZT_Element_Defaults *breakable_def = zzt_get_element_defaults(ZZT_BREAKABLE);
    if (!b || !breakable_def)
        return;

    Bzzt_Tile blast_tile = Bzzt_Board_Get_Tile(b, x, y);
    blast_tile.element = ZZT_BREAKABLE;
    blast_tile.glyph = breakable_def->default_glyph;
    blast_tile.fg = bzzt_get_color(9 + (rand() % 7));
    blast_tile.visible = true;
    blast_tile.blink = false;
    Bzzt_Board_Set_Tile(b, x, y, blast_tile);
}

// Phase 1: spawn breakables and kill destructible enemies in the blast radius.
static void zzt_bomb_explode(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *bomb_stat)
{
    int bx = bomb_stat->x;
    int by = bomb_stat->y;

    for (int dy = -BZZT_RADIUS_MAX_DY; dy <= BZZT_RADIUS_MAX_DY; dy++)
    {
        for (int dx = -BZZT_RADIUS_MAX_DX; dx <= BZZT_RADIUS_MAX_DX; dx++)
        {
            if (!bzzt_in_radius(dx, dy))
                continue;
            int tx = bx + dx;
            int ty = by + dy;
            if (!Bzzt_Board_Is_In_Bounds(b, tx, ty))
                continue;

            Bzzt_Tile t = Bzzt_Board_Get_Tile(b, tx, ty);
            Bzzt_Stat *s = Bzzt_Board_Get_Stat_At(b, tx, ty);
            bool spawn_blast = false;

            if (s)
            {
                if (t.element == ZZT_PLAYER)
                {
                    w->health -= 10;
                    UI_Flash_Message(ui, ZZT_MSG_OUCH);
                }
                else if (s != bomb_stat && bomb_element_is_destructible(t.element))
                {
                    Bzzt_Board_Stat_Die(b, s);
                    t = Bzzt_Board_Get_Tile(b, tx, ty);
                    spawn_blast = (t.element == ZZT_EMPTY || t.element == ZZT_FAKE);
                }

                if (spawn_blast)
                    spawn_bomb_blast_tile(b, tx, ty);
                continue;
            }

            if (t.element == ZZT_BREAKABLE)
            {
                Bzzt_Board_Set_Tile(b, tx, ty, empty_tile);
                spawn_blast = true;
            }
            else if (t.element == ZZT_EMPTY || t.element == ZZT_FAKE)
            {
                spawn_blast = true;
            }

            if (spawn_blast)
                spawn_bomb_blast_tile(b, tx, ty);
        }
    }
}

// Phase 2: clear breakables spawned by the explosion, then bomb dies.
static void zzt_bomb_cleanup(Bzzt_Board *b, Bzzt_Stat *bomb_stat)
{
    int bx = bomb_stat->x;
    int by = bomb_stat->y;

    for (int dy = -BZZT_RADIUS_MAX_DY; dy <= BZZT_RADIUS_MAX_DY; dy++)
    {
        for (int dx = -BZZT_RADIUS_MAX_DX; dx <= BZZT_RADIUS_MAX_DX; dx++)
        {
            if (!bzzt_in_radius(dx, dy))
                continue;
            int tx = bx + dx;
            int ty = by + dy;
            if (!Bzzt_Board_Is_In_Bounds(b, tx, ty))
                continue;

            Bzzt_Tile t = Bzzt_Board_Get_Tile(b, tx, ty);
            if (t.element == ZZT_BREAKABLE)
            {
                t.element = ZZT_EMPTY;
                t.glyph = ' ';
                Bzzt_Board_Set_Tile(b, tx, ty, t);
            }
        }
    }
    Bzzt_Board_Stat_Die(b, bomb_stat);
}

static void zzt_bomb_tick(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (stat->data[0] == 0)
        return; // inactive

    // data[0] == 255 is the cleanup sentinel (one tick after explosion)
    if (stat->data[0] == 255)
    {
        zzt_bomb_cleanup(b, stat);
        return;
    }

    stat->data[0]--;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);

    if (stat->data[0] <= 1)
    {
        // Explosion tick — glyph reverts to ♂ (11) briefly
        tile.glyph = 11;
        Bzzt_Board_Set_Tile(b, stat->x, stat->y, tile);
        zzt_bomb_explode(ui, w, b, stat);
        stat->data[0] = 255; // arm cleanup for next tick
        return;
    }

    tile.glyph = '0' + stat->data[0];
    Bzzt_Board_Set_Tile(b, stat->x, stat->y, tile);
}

// tbd
static void zzt_pusher_tick(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    (void)w;

    Direction dir;
    int dx = 0;
    int dy = 0;

    if (stat->step_x > 0)
    {
        dir = DIR_RIGHT;
        dx = 1;
    }
    else if (stat->step_x < 0)
    {
        dir = DIR_LEFT;
        dx = -1;
    }
    else if (stat->step_y > 0)
    {
        dir = DIR_DOWN;
        dy = 1;
    }
    else if (stat->step_y < 0)
    {
        dir = DIR_UP;
        dy = -1;
    }
    else
        return;

    int next_x = stat->x + dx;
    int next_y = stat->y + dy;

    if (!Bzzt_Board_Is_In_Bounds(b, next_x, next_y))
        return;

    Bzzt_Tile next_tile = Bzzt_Board_Get_Tile(b, next_x, next_y);
    if (Bzzt_Tile_Is_Pushable(next_tile))
        push_tile(b, dir, next_tile);

    next_tile = Bzzt_Board_Get_Tile(b, next_x, next_y);
    if (next_tile.element == ZZT_EMPTY || next_tile.element == ZZT_FAKE)
        Bzzt_Board_Move_Stat_To(b, stat, next_x, next_y);
}

static void stat_tick(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !b || !stat)
        return;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);
    uint8_t stat_type = tile.element;

    switch (stat_type)
    {
    case ZZT_MONITOR:
        break;

    case ZZT_PLAYER:
        zzt_player_tick(ui, w, stat);
        break;

    case ZZT_BULLET:
        zzt_bullet_tick(ui, w, b, stat);
        break;

    case ZZT_SPINNINGGUN:
        zzt_spinninggun_tick(w, b, stat, tile);
        break;

    case ZZT_BOMB:
        zzt_bomb_tick(ui, w, b, stat);
        break;

    case ZZT_PUSHER:
        zzt_pusher_tick(w, b, stat);
        break;

    default:
        break;
    }
}

Vector2 vector2_from_direction(Direction direction)
{
    switch (direction)
    {
    case DIR_NONE:
        return (Vector2){0, 0};
    case DIR_UP:
        return (Vector2){0, -1};
    case DIR_DOWN:
        return (Vector2){0, 1};
    case DIR_LEFT:
        return (Vector2){-1, 0};
    case DIR_RIGHT:
        return (Vector2){1, 0};
    }
}

void push_tile(Bzzt_Board *b, Direction direction, Bzzt_Tile tile)
{
    if (!Bzzt_Tile_Is_Pushable(tile))
        return;

    if (tile.element == ZZT_EWSLIDER && (direction == DIR_UP || direction == DIR_DOWN))
        return;
    if (tile.element == ZZT_NSSLIDER && (direction == DIR_LEFT || direction == DIR_RIGHT))
        return;

    int max_len = b->width - 1;

    Bzzt_Tile chain[max_len];
    int chain_len = 0;
    chain[chain_len++] = tile;

    Vector2 vec = vector2_from_direction(direction);
    Bzzt_Tile t = tile;
    bool can_push = false;

    // scan for chain of pushable objects
    while (true)
    {
        int next_x = t.x + (int)vec.x;
        int next_y = t.y + (int)vec.y;
        if (!Bzzt_Board_Is_In_Bounds(b, next_x, next_y))
            break; // hit board edge

        t = Bzzt_Board_Get_Tile(b, next_x, next_y);
        if (t.element == ZZT_EWSLIDER)
        {
            if (direction == DIR_LEFT || direction == DIR_RIGHT)
                chain[chain_len++] = t;
            else
                break;
        }
        else if (t.element == ZZT_NSSLIDER)
        {
            if (direction == DIR_UP || direction == DIR_DOWN)
                chain[chain_len++] = t;
            else
                break;
        }
        else if (Bzzt_Tile_Is_Pushable(t))
            chain[chain_len++] = t;
        else if (t.element == ZZT_EMPTY || t.element == ZZT_FAKE)
        {
            can_push = true;
            break;
        }
        else
            break; // solid wall or other blocker
    }

    if (!can_push)
        return;

    // push from tail to head
    for (int i = chain_len - 1; i >= 0; --i)
    {
        Bzzt_Tile src = chain[i];
        int dest_x = src.x + (int)vec.x;
        int dest_y = src.y + (int)vec.y;

        Bzzt_Stat *stat = Bzzt_Board_Get_Stat_At(b, src.x, src.y);
        if (stat)
            Bzzt_Board_Move_Stat_To(b, stat, dest_x, dest_y);
        else
            Bzzt_Board_Move_Tile_To(b, src, dest_x, dest_y);
    }
}

void zzt_bullet_tick(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    int next_x = stat->x + stat->step_x;
    int next_y = stat->y + stat->step_y;

    if (!Bzzt_Board_Is_In_Bounds(b, next_x, next_y))
    {
        Bzzt_Board_Stat_Die(b, stat);
        return;
    }

    Bzzt_Tile next_tile = Bzzt_Board_Get_Tile(b, next_x, next_y);
    if (next_tile.element != ZZT_EMPTY && next_tile.element != ZZT_FAKE && next_tile.element != ZZT_WATER)
        handle_bullet_collision(ui, w, b, stat, next_x, next_y);
    else
        Bzzt_Board_Move_Stat_To(b, stat, next_x, next_y);
}

void zzt_spinninggun_tick(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat, Bzzt_Tile tile)
{
    // tbd - add star firing
    // feels pretty accurate to zzt behavior but maybe isn't
    //  Gun changes char every 2 ticks
    int anim_phase = (w->timer->current_tick / 2) % 4;
    tile.glyph = (anim_phase == 0) ? 24 : (anim_phase == 1) ? 26
                                      : (anim_phase == 2)   ? 25
                                                            : 27;
    Bzzt_Board_Set_Tile(b, stat->x, stat->y, tile);

    // Calculate firing probabilities
    double chance_of_fire = (double)(stat->data[1]) / 9.0;
    double chance_of_smart_fire = (double)(stat->data[0] + 1) / 9.0;
    double roll = ((double)rand()) / RAND_MAX;
    double roll_smart = ((double)rand()) / RAND_MAX;

    // Determine if we should fire
    bool should_fire = roll < chance_of_fire;
    bool fire_intelligently = roll_smart < chance_of_smart_fire && should_fire;

    if (should_fire)
    {
        Direction fire_dir = DIR_NONE;
        Bzzt_Stat *player = b->stats[0];

        if (player && fire_intelligently)
        {
            // Intelligent targeting logic
            int dx = player->x - stat->x;
            int dy = player->y - stat->y;
            int abs_dx = (dx < 0) ? -dx : dx;
            int abs_dy = (dy < 0) ? -dy : dy;

            // Try vertical shot if player is within 2 tiles horizontally
            if (abs_dx <= 2)
            {
                fire_dir = (dy > 0) ? DIR_DOWN : DIR_UP;
                // Check if vertical path is clear
                if (Bzzt_Stat_Is_Blocked(w, b, stat, fire_dir))
                    fire_dir = DIR_NONE;
            }

            // Try horizontal shot if player is within 2 tiles vertically
            if (fire_dir == DIR_NONE && abs_dy <= 2)
            {
                fire_dir = (dx > 0) ? DIR_RIGHT : DIR_LEFT;
                // Check if horizontal path is clear
                if (Bzzt_Stat_Is_Blocked(w, b, stat, fire_dir))
                    fire_dir = DIR_NONE; // Path blocked, will use random
            }

            if (fire_dir == DIR_NONE)
            {
                int random_dir = rand() % 4;
                fire_dir = (random_dir == 0) ? DIR_UP : (random_dir == 1) ? DIR_RIGHT
                                                    : (random_dir == 2)   ? DIR_DOWN
                                                                          : DIR_LEFT;
            }
        }
        else if (should_fire)
        {
            // Non-intelligent: random cardinal direction
            int random_dir = rand() % 4;
            fire_dir = (random_dir == 0) ? DIR_UP : (random_dir == 1) ? DIR_RIGHT
                                                : (random_dir == 2)   ? DIR_DOWN
                                                                      : DIR_LEFT;
        }
        // Fire the shot
        if (fire_dir != DIR_NONE)
            Bzzt_Stat_Shoot(b, stat, fire_dir);
    }
}

void zzt_player_tick(UI *ui, Bzzt_World *w, Bzzt_Stat *player_stat)
{
    handle_player_move(ui, w);
    handle_player_shoot(ui, w, player_stat);
}

Bzzt_Stat *Bzzt_Stat_Create(Bzzt_Board *b, int x, int y)
{
    Bzzt_Stat *s = malloc(sizeof(Bzzt_Stat));
    if (!s)
        return NULL;

    s->x = x;
    s->y = y;
    s->step_x = 0;
    s->step_y = 0;
    s->prev_x = 0;
    s->prev_y = 0;
    s->cycle = 0;
    s->data[0] = 0;
    s->data[1] = 0;
    s->data[2] = 0;
    s->data_label[0] = 0;
    s->data_label[1] = 0;
    s->data_label[2] = 0;
    s->follower = -1;
    s->leader = -1;
    s->under = Bzzt_Board_Get_Tile(b, x, y);
    s->program = NULL;
    s->program_length = 0;
    s->program_counter = 0;
    return s;
}

void Bzzt_Stat_Destroy(Bzzt_Stat *s)
{
    if (!s)
        return;
    if (s->program)
        free(s->program);
    free(s);
}

void Bzzt_Stat_Update(UI *ui, Bzzt_World *w, Bzzt_Stat *stat, int stat_idx)
{
    if (!w || !stat)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];

    if (stat_can_act(w, stat, stat_idx))
        stat_tick(ui, w, current_board, stat);
}

bool Bzzt_Tile_Is_Walkable(Bzzt_World *w, Bzzt_Tile tile)
{
    switch (tile.element)
    {
    case ZZT_EMPTY:
    case ZZT_FAKE:
    case ZZT_TORCH:
    case ZZT_AMMO:
    case ZZT_GEM:
    case ZZT_ENERGIZER:
    case ZZT_FOREST:
        return true;
    case ZZT_KEY:
        return !player_has_key(w, tile); // walkable only if player doesn't already have this color
    default:
        return false;
        break;
    }
}

bool Bzzt_Tile_Is_Pushable(Bzzt_Tile tile)
{
    // Only certain elements can be pushed by bolders, sliders, etc
    switch (tile.element)
    {
    case ZZT_PLAYER:
    case ZZT_GEM:
    case ZZT_AMMO:
    case ZZT_BOMB:
    case ZZT_KEY:
    case ZZT_SCROLL:
    case ZZT_BOULDER:
    case ZZT_EWSLIDER:
    case ZZT_NSSLIDER:
        return true;
    default:
        return false;
    }
}

bool Bzzt_Tile_Is_Blocked(Bzzt_Board *b, Bzzt_Tile tile, Direction direction)
{
    Vector2 vec = vector2_from_direction(direction);
    int dx = tile.x + (int)vec.x;
    int dy = tile.y + (int)vec.y;

    if (!Bzzt_Board_Is_In_Bounds(b, dx, dy))
        return true;

    Bzzt_Tile neighbor = Bzzt_Board_Get_Tile(b, dx, dy);
    return neighbor.element != ZZT_EMPTY && neighbor.element != ZZT_FAKE;
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

Bzzt_Tile Bzzt_Tile_From_ZZT_Tile(ZZTblock *block, int x, int y)
{
    Bzzt_Tile tile = {0};
    if (!block || x < 0 || x > block->width || y < 0 || y > block->height)
        return tile;
    tile.glyph = zztTileGetDisplayChar(block, x, y);
    uint8_t attr = zztTileGetDisplayColor(block, x, y);

    uint8_t fg_idx = attr & 0x0F;
    uint8_t bg_idx = (attr >> 4) & 0x07; // 3 bits: background is 0-7 in EGA
    bool blink = (attr & 0x80) != 0;     // bit 7: blink flag

    tile.fg = bzzt_get_color(fg_idx);
    tile.bg = bzzt_get_color(bg_idx);
    tile.blink = blink;

    ZZTtile zzt_tile = zztTileAt(block, x, y);
    tile.element = zzt_tile.type;

    tile.x = x;
    tile.y = y;

    if (tile.element == ZZT_INVISIBLE)
        tile.visible = false;
    else
        tile.visible = true;

    return tile;
}

Bzzt_Stat *Bzzt_Stat_From_ZZT_Param(ZZTparam *param, ZZTtile tile, int x, int y)
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
    stat->prev_x = x;
    stat->prev_y = y;

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
    uint8_t bg_idx = (param->ucolor >> 4) & 0x07; // 3 bits: background is 0-7 in EGA
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

// cursed and forbidden
void Bzzt_Get_Interpolated_Position(Bzzt_World *w, Bzzt_Stat *stat, float *out_x, float *out_y)
{
    if (!w || !stat || !out_x || !out_y)
        return;

#if BZZT_ENABLE_INTERPOLATION
    if (w->interpolation_enabled && w->timer)
    {
        // Calculate interpolation factor (0.0 to 1.0)
        double t = w->timer->accumulator_ms / w->timer->tick_duration_ms;

        // Clamp to valid range
        if (t < 0.0)
            t = 0.0;
        if (t > 1.0)
            t = 1.0;

        // Linear interpolation between prev and current position
        *out_x = (float)stat->prev_x + (float)(stat->x - stat->prev_x) *
                                           (float)t;
        *out_y = (float)stat->prev_y + (float)(stat->y - stat->prev_y) *
                                           (float)t;
    }
    else
#endif
    {
        // No interpolation - use exact position
        *out_x = (float)stat->x;
        *out_y = (float)stat->y;
    }
}
