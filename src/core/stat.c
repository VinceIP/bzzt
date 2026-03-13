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
Vector2 vector2_from_direction(Direction direction);
static bool transporter_try_move_stat(Bzzt_Board *b, Bzzt_Stat *stat, int transporter_x, int transporter_y, Direction move_dir);
void push_tile(Bzzt_Board *b, Direction direction, Bzzt_Tile tile);

static bool world_is_on_title_screen(Bzzt_World *w)
{
    return w && w->on_title;
}

static Direction opposite_direction(Direction dir)
{
    switch (dir)
    {
    case DIR_UP:
        return DIR_DOWN;
    case DIR_DOWN:
        return DIR_UP;
    case DIR_LEFT:
        return DIR_RIGHT;
    case DIR_RIGHT:
        return DIR_LEFT;
    default:
        return DIR_NONE;
    }
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

    if (target_tile.element == ZZT_TRANSPORTER && transporter_try_move_stat(current_board, stat, new_x, new_y, move_dir))
    {
        if (dx != 0)
            stat->step_x = (dx > 0) ? 1 : -1;
        if (dy != 0)
            stat->step_y = (dy > 0) ? 1 : -1;
        if (w->paused)
            Bzzt_World_Set_Pause(w, false);
        return;
    }

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
    blast_tile.bg = COLOR_BLACK;
    blast_tile.visible = true;
    blast_tile.blink = false;
    Bzzt_Board_Set_Tile(b, x, y, blast_tile);
}

static Direction blink_wall_direction(Bzzt_Stat *stat)
{
    if (!stat)
        return DIR_NONE;

    if (stat->step_x > 0)
        return DIR_RIGHT;
    if (stat->step_x < 0)
        return DIR_LEFT;
    if (stat->step_y > 0)
        return DIR_DOWN;
    if (stat->step_y < 0)
        return DIR_UP;

    return DIR_NONE;
}

static uint8_t blink_wall_ray_element(Direction dir)
{
    return (dir == DIR_LEFT || dir == DIR_RIGHT) ? ZZT_BLINKHORIZ : ZZT_BLINKVERT;
}

static bool tile_is_blink_ray_for_direction(Bzzt_Tile tile, Direction dir)
{
    return tile.element == blink_wall_ray_element(dir);
}

static bool tile_is_walkable_for_blink_escape(Bzzt_World *w, Bzzt_Board *b, int x, int y)
{
    if (!b || !Bzzt_Board_Is_In_Bounds(b, x, y))
        return false;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
    return Bzzt_Tile_Is_Walkable(w, tile);
}

static bool move_player_out_of_blink_ray(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *player, Direction ray_dir)
{
    if (!w || !b || !player)
        return false;

    int dest_x = player->x;
    int dest_y = player->y;

    if (ray_dir == DIR_UP || ray_dir == DIR_DOWN)
    {
        bool east_open = tile_is_walkable_for_blink_escape(w, b, player->x + 1, player->y);
        bool west_open = tile_is_walkable_for_blink_escape(w, b, player->x - 1, player->y);

        if (east_open)
            dest_x = player->x + 1;
        else if (west_open)
            dest_x = player->x + 1; // Original ZZT bug: west is checked, but east is still used.
        else
        {
            w->health = 0;
            UI_Flash_Message(ui, ZZT_MSG_OUCH);
            return false;
        }
    }
    else
    {
        bool north_open = tile_is_walkable_for_blink_escape(w, b, player->x, player->y - 1);
        bool south_open = tile_is_walkable_for_blink_escape(w, b, player->x, player->y + 1);

        if (north_open)
            dest_y = player->y - 1;
        else if (south_open)
            dest_y = player->y + 1;
        else
        {
            w->health = 0;
            UI_Flash_Message(ui, ZZT_MSG_OUCH);
            return false;
        }
    }

    w->health -= 10;
    UI_Flash_Message(ui, ZZT_MSG_OUCH);
    Bzzt_Board_Move_Stat_To(b, player, dest_x, dest_y);
    return true;
}

static bool blink_wall_hits_creature(Bzzt_World *w, Bzzt_Tile tile)
{
    (void)w;

    switch (tile.element)
    {
    case ZZT_BEAR:
    case ZZT_RUFFIAN:
    case ZZT_OBJECT:
    case ZZT_SLIME:
    case ZZT_SHARK:
    case ZZT_SPINNINGGUN:
    case ZZT_PUSHER:
    case ZZT_LION:
    case ZZT_TIGER:
    case ZZT_CENTHEAD:
    case ZZT_CENTBODY:
    case ZZT_BULLET:
    case ZZT_STAR:
    case ZZT_BOMB:
    case ZZT_PLAYER:
        return true;
    default:
        return false;
    }
}

static bool blink_wall_handle_obstruction(UI *ui, Bzzt_World *w, Bzzt_Board *b, Direction ray_dir, int x, int y)
{
    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
    Bzzt_Stat *stat = Bzzt_Board_Get_Stat_At(b, x, y);

    if (!stat)
        return false;

    if (tile.element == ZZT_PLAYER && stat == b->stats[0])
    {
        move_player_out_of_blink_ray(ui, w, b, stat, ray_dir);
        return true;
    }

    if (blink_wall_hits_creature(w, tile))
    {
        Bzzt_Board_Stat_Die(b, stat);
        return true;
    }

    return true;
}

static void clear_blink_wall_rays(Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!b || !stat)
        return;

    Direction dir = blink_wall_direction(stat);
    Vector2 vec = vector2_from_direction(dir);
    uint8_t ray_element = blink_wall_ray_element(dir);
    int x = stat->x + (int)vec.x;
    int y = stat->y + (int)vec.y;

    while (Bzzt_Board_Is_In_Bounds(b, x, y))
    {
        Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
        if (tile.element != ray_element)
            break;

        Bzzt_Board_Set_Tile(b, x, y, empty_tile);
        x += (int)vec.x;
        y += (int)vec.y;
    }
}

static void create_blink_wall_rays(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !b || !stat)
        return;

    Direction dir = blink_wall_direction(stat);
    Vector2 vec = vector2_from_direction(dir);
    uint8_t ray_element = blink_wall_ray_element(dir);
    Bzzt_Tile wall_tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);

    int x = stat->x + (int)vec.x;
    int y = stat->y + (int)vec.y;

    while (Bzzt_Board_Is_In_Bounds(b, x, y))
    {
        Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
        if (tile.element != ZZT_EMPTY)
        {
            blink_wall_handle_obstruction(ui, w, b, dir, x, y);
            break;
        }

        Bzzt_Tile ray_tile = tile;
        ray_tile.element = ray_element;
        ray_tile.glyph = zzt_type_to_cp437(ray_element, 0);
        ray_tile.fg = wall_tile.fg;
        ray_tile.bg = wall_tile.bg;
        ray_tile.visible = true;
        ray_tile.blink = false;
        Bzzt_Board_Set_Tile(b, x, y, ray_tile);

        x += (int)vec.x;
        y += (int)vec.y;
    }
}

static void zzt_blink_wall_tick(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !b || !stat)
        return;

    Direction dir = blink_wall_direction(stat);
    if (dir == DIR_NONE)
        return;

    if (stat->data[0] > 0)
    {
        stat->data[0]--;
        if (stat->data[0] > 0)
            return;
    }

    Vector2 vec = vector2_from_direction(dir);
    int first_x = stat->x + (int)vec.x;
    int first_y = stat->y + (int)vec.y;

    if (Bzzt_Board_Is_In_Bounds(b, first_x, first_y) &&
        tile_is_blink_ray_for_direction(Bzzt_Board_Get_Tile(b, first_x, first_y), dir))
        clear_blink_wall_rays(b, stat);
    else
        create_blink_wall_rays(ui, w, b, stat);

    stat->data[0] = stat->data[1];
}

static bool transporter_faces_direction(Bzzt_Stat *transporter, Direction move_dir)
{
    return blink_wall_direction(transporter) == move_dir;
}

static bool transporter_try_open_destination(Bzzt_Board *b, int dest_x, int dest_y, Direction move_dir)
{
    if (!b || !Bzzt_Board_Is_In_Bounds(b, dest_x, dest_y))
        return false;

    Bzzt_Tile dest_tile = Bzzt_Board_Get_Tile(b, dest_x, dest_y);
    if (Bzzt_Tile_Is_Pushable(dest_tile))
        push_tile(b, move_dir, dest_tile);

    dest_tile = Bzzt_Board_Get_Tile(b, dest_x, dest_y);
    return dest_tile.element == ZZT_EMPTY || dest_tile.element == ZZT_FAKE;
}

static bool transporter_resolve_exit(Bzzt_Board *b, int transporter_x, int transporter_y, Direction move_dir, int *out_x, int *out_y)
{
    int x = transporter_x;
    int y = transporter_y;
    Vector2 vec = vector2_from_direction(move_dir);
    int one_way_x = transporter_x + (int)vec.x;
    int one_way_y = transporter_y + (int)vec.y;

    if (transporter_try_open_destination(b, one_way_x, one_way_y, move_dir))
    {
        *out_x = one_way_x;
        *out_y = one_way_y;
        return true;
    }

    x = one_way_x;
    y = one_way_y;

    while (Bzzt_Board_Is_In_Bounds(b, x, y))
    {
        Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
        Bzzt_Stat *transporter = Bzzt_Board_Get_Stat_At(b, x, y);

        if (tile.element == ZZT_TRANSPORTER &&
            transporter &&
            transporter_faces_direction(transporter, opposite_direction(move_dir)))
        {
            int dest_x = x + (int)vec.x;
            int dest_y = y + (int)vec.y;

            if (transporter_try_open_destination(b, dest_x, dest_y, move_dir))
            {
                *out_x = dest_x;
                *out_y = dest_y;
                return true;
            }
        }

        x += (int)vec.x;
        y += (int)vec.y;
    }

    return false;
}

static void update_transporter_glyph(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !w->timer || !b || !stat)
        return;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);
    int phase = (stat->cycle > 0) ? ((w->timer->current_tick / stat->cycle) % 4) : 0;
    Direction dir = blink_wall_direction(stat);

    switch (dir)
    {
    case DIR_UP:
        tile.glyph = (phase == 0 || phase == 2) ? '^' : (phase == 1) ? '~'
                                                                      : '-';
        break;
    case DIR_DOWN:
        tile.glyph = (phase == 0 || phase == 2) ? 'v' : (phase == 1) ? '_'
                                                                      : '-';
        break;
    case DIR_RIGHT:
        tile.glyph = (phase == 0 || phase == 2) ? '(' : (phase == 1) ? '<'
                                                                      : 179;
        break;
    case DIR_LEFT:
        tile.glyph = (phase == 0 || phase == 2) ? ')' : (phase == 1) ? '>'
                                                                      : 179;
        break;
    default:
        tile.glyph = zzt_type_to_cp437(ZZT_TRANSPORTER, 0);
        break;
    }

    Bzzt_Board_Set_Tile(b, stat->x, stat->y, tile);
}

static bool transporter_try_move_stat(Bzzt_Board *b, Bzzt_Stat *stat, int transporter_x, int transporter_y, Direction move_dir)
{
    int dest_x = 0;
    int dest_y = 0;
    if (!b || !stat)
        return false;

    Bzzt_Stat *transporter = Bzzt_Board_Get_Stat_At(b, transporter_x, transporter_y);
    if (!transporter || !transporter_faces_direction(transporter, move_dir))
        return false;

    if (!transporter_resolve_exit(b, transporter_x, transporter_y, move_dir, &dest_x, &dest_y))
        return false;

    Bzzt_Board_Move_Stat_To(b, stat, dest_x, dest_y);
    return true;
}

static bool transporter_try_move_tile(Bzzt_Board *b, Bzzt_Tile tile, int transporter_x, int transporter_y, Direction move_dir)
{
    int dest_x = 0;
    int dest_y = 0;
    Bzzt_Stat *transporter;

    if (!b)
        return false;

    transporter = Bzzt_Board_Get_Stat_At(b, transporter_x, transporter_y);
    if (!transporter || !transporter_faces_direction(transporter, move_dir))
        return false;

    if (!transporter_resolve_exit(b, transporter_x, transporter_y, move_dir, &dest_x, &dest_y))
        return false;

    Bzzt_Board_Move_Tile_To(b, tile, dest_x, dest_y);
    return true;
}

static Bzzt_Stat *clone_stat_for_duplication(Bzzt_Stat *source, Bzzt_Tile under, int x, int y)
{
    if (!source)
        return NULL;

    Bzzt_Stat *clone = malloc(sizeof(Bzzt_Stat));
    if (!clone)
        return NULL;

    memcpy(clone, source, sizeof(Bzzt_Stat));
    clone->x = x;
    clone->y = y;
    clone->prev_x = x;
    clone->prev_y = y;
    clone->under = under;
    clone->leader = -1;
    clone->follower = -1;

    if (source->program && source->program_length > 0)
    {
        clone->program = malloc(source->program_length + 1);
        if (!clone->program)
        {
            free(clone);
            return NULL;
        }

        memcpy(clone->program, source->program, source->program_length + 1);
    }
    else
    {
        clone->program = NULL;
        clone->program_length = 0;
        clone->program_counter = 0;
    }

    return clone;
}

static int duplicator_interval(Bzzt_Stat *stat)
{
    if (!stat)
        return 27;

    int rate = stat->data[1];
    if (rate < 0)
        rate = 0;
    if (rate > 8)
        rate = 8;
    return (9 - rate) * 3;
}

static void update_duplicator_glyph(Bzzt_Board *b, Bzzt_Stat *stat)
{
    static const uint8_t seq[] = {250, 250, 249, 248, 111, 79};

    if (!b || !stat)
        return;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);
    if (stat->data[0] >= 1 && stat->data[0] <= 5)
        tile.glyph = seq[stat->data[0]];
    else
        tile.glyph = 250;
    Bzzt_Board_Set_Tile(b, stat->x, stat->y, tile);
}

static void zzt_duplicator_tick(UI *ui, Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !b || !stat)
        return;

    stat->cycle = duplicator_interval(stat);
    if (stat->data[0] < 255)
        stat->data[0]++;
    update_duplicator_glyph(b, stat);

    Direction dir = blink_wall_direction(stat);
    if (dir == DIR_NONE)
    {
        stat->data[0] = 0;
        return;
    }

    Vector2 vec = vector2_from_direction(dir);
    Direction output_dir = opposite_direction(dir);
    int src_x = stat->x + (int)vec.x;
    int src_y = stat->y + (int)vec.y;
    int dest_x = stat->x - (int)vec.x;
    int dest_y = stat->y - (int)vec.y;

    if (!Bzzt_Board_Is_In_Bounds(b, src_x, src_y) || !Bzzt_Board_Is_In_Bounds(b, dest_x, dest_y))
    {
        return;
    }

    Bzzt_Tile src_tile = Bzzt_Board_Get_Tile(b, src_x, src_y);
    Bzzt_Stat *src_stat = Bzzt_Board_Get_Stat_At(b, src_x, src_y);
    if (src_tile.element == ZZT_PLAYER || src_tile.element == ZZT_MONITOR)
    {
        stat->data[0] = 0;
        update_duplicator_glyph(b, stat);
        return;
    }

    Bzzt_Tile dest_tile = Bzzt_Board_Get_Tile(b, dest_x, dest_y);
    if (stat->data[0] < 5)
        return;

    Bzzt_Stat *dest_stat = Bzzt_Board_Get_Stat_At(b, dest_x, dest_y);
    if (dest_tile.element == ZZT_PLAYER && dest_stat == b->stats[0])
    {
        // TODO: Objects likely need extra duplication/touch quirks once ZZT-OOP support is expanded.
        handle_player_touch(ui, w, src_tile, src_x, src_y, dir);
        stat->data[0] = 0;
        update_duplicator_glyph(b, stat);
        return;
    }

    if (Bzzt_Tile_Is_Pushable(dest_tile))
        push_tile(b, output_dir, dest_tile);

    dest_tile = Bzzt_Board_Get_Tile(b, dest_x, dest_y);
    if (dest_tile.element != ZZT_EMPTY && dest_tile.element != ZZT_FAKE)
    {
        stat->data[0] = 0;
        update_duplicator_glyph(b, stat);
        return;
    }

    if (src_stat)
    {
        Bzzt_Stat *clone = clone_stat_for_duplication(src_stat, dest_tile, dest_x, dest_y);
        if (!clone || !Bzzt_Board_Add_Stat(b, clone))
        {
            if (clone)
            {
                if (clone->program)
                    free(clone->program);
                free(clone);
            }
            stat->data[0] = 0;
            update_duplicator_glyph(b, stat);
            return;
        }
    }
    else
    {
        Bzzt_Stat *existing_dest_stat = Bzzt_Board_Get_Stat_At(b, dest_x, dest_y);
        if (existing_dest_stat)
            Bzzt_Board_Stat_Die(b, existing_dest_stat);
    }

    Bzzt_Tile out_tile = src_tile;
    out_tile.x = dest_x;
    out_tile.y = dest_y;
    if (src_stat && out_tile.element != ZZT_PLAYER)
        out_tile.bg = dest_tile.bg;
    Bzzt_Board_Set_Tile(b, dest_x, dest_y, out_tile);

    stat->data[0] = 0;
    update_duplicator_glyph(b, stat);
}

typedef struct ConveyorRingCell
{
    int x;
    int y;
    bool in_bounds;
    Bzzt_Tile tile;
    Bzzt_Stat *stat;
} ConveyorRingCell;

static const int conveyor_ring_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const int conveyor_ring_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};

static bool conveyor_cell_is_emptyish(ConveyorRingCell cell)
{
    return cell.in_bounds &&
           (cell.tile.element == ZZT_EMPTY || cell.tile.element == ZZT_FAKE);
}

static bool conveyor_cell_is_pushable(ConveyorRingCell cell)
{
    return cell.in_bounds && Bzzt_Tile_Is_Pushable(cell.tile);
}

static bool conveyor_cell_is_blocker(ConveyorRingCell cell)
{
    return !cell.in_bounds || (!conveyor_cell_is_emptyish(cell) && !conveyor_cell_is_pushable(cell));
}

static void build_conveyor_ring(Bzzt_Board *b, Bzzt_Stat *stat, ConveyorRingCell ring[8])
{
    for (int i = 0; i < 8; ++i)
    {
        int x = stat->x + conveyor_ring_dx[i];
        int y = stat->y + conveyor_ring_dy[i];

        ring[i].x = x;
        ring[i].y = y;
        ring[i].in_bounds = Bzzt_Board_Is_In_Bounds(b, x, y);
        ring[i].tile = ring[i].in_bounds ? Bzzt_Board_Get_Tile(b, x, y) : empty_tile;
        ring[i].stat = ring[i].in_bounds ? Bzzt_Board_Get_Stat_At(b, x, y) : NULL;
    }
}

static int wrap_ring_index(int idx)
{
    while (idx < 0)
        idx += 8;
    while (idx >= 8)
        idx -= 8;
    return idx;
}

static void conveyor_shift_linear_segment(int working[8], const int indices[], int len)
{
    for (int pos = len - 1; pos > 0; --pos)
    {
        int dest = indices[pos];
        int src = indices[pos - 1];

        if (working[dest] == -1 && working[src] >= 0)
        {
            working[dest] = working[src];
            working[src] = -1;
        }
    }
}

static void compute_conveyor_targets(const ConveyorRingCell ring[8], bool clockwise, int targets[8])
{
    int working[8];
    int blocker_count = 0;

    for (int i = 0; i < 8; ++i)
    {
        if (conveyor_cell_is_blocker(ring[i]))
        {
            working[i] = -2;
            blocker_count++;
        }
        else if (conveyor_cell_is_pushable(ring[i]))
            working[i] = i;
        else
            working[i] = -1;
    }

    if (blocker_count == 0)
    {
        for (int i = 0; i < 8; ++i)
        {
            int source = clockwise ? wrap_ring_index(i - 1) : wrap_ring_index(i + 1);
            targets[i] = (working[source] >= 0) ? working[source] : -1;
        }
        return;
    }

    for (int i = 0; i < 8; ++i)
        targets[i] = (working[i] >= 0) ? working[i] : -1;

    int blockers[8];
    for (int i = 0; i < 8; ++i)
    {
        if (working[i] == -2)
            blockers[i] = 1;
        else
            blockers[i] = 0;
    }

    for (int blocker_idx = 0; blocker_idx < 8; ++blocker_idx)
    {
        if (!blockers[blocker_idx])
            continue;

        int indices[8];
        int len = 0;
        int cursor = blocker_idx;

        while (true)
        {
            cursor = clockwise ? wrap_ring_index(cursor + 1) : wrap_ring_index(cursor - 1);
            if (working[cursor] == -2)
                break;
            indices[len++] = cursor;
        }

        if (len == 0)
            continue;

        conveyor_shift_linear_segment(working, indices, len);
    }

    for (int i = 0; i < 8; ++i)
        targets[i] = (working[i] >= 0) ? working[i] : -1;
}

static void apply_conveyor_targets(Bzzt_Board *b, const ConveyorRingCell ring[8], const int targets[8])
{
    Bzzt_Tile base_tiles[8];

    for (int i = 0; i < 8; ++i)
    {
        if (!ring[i].in_bounds)
            continue;

        if (ring[i].stat)
            base_tiles[i] = ring[i].stat->under;
        else if (conveyor_cell_is_pushable(ring[i]))
            base_tiles[i] = empty_tile;
        else
            base_tiles[i] = ring[i].tile;

        Bzzt_Board_Set_Tile(b, ring[i].x, ring[i].y, base_tiles[i]);
    }

    for (int i = 0; i < 8; ++i)
    {
        int source_idx = targets[i];
        if (!ring[i].in_bounds || source_idx < 0)
            continue;

        ConveyorRingCell source = ring[source_idx];
        Bzzt_Tile moved_tile = source.tile;
        moved_tile.x = ring[i].x;
        moved_tile.y = ring[i].y;

        if (source.stat)
        {
            source.stat->prev_x = source.stat->x;
            source.stat->prev_y = source.stat->y;
            source.stat->x = ring[i].x;
            source.stat->y = ring[i].y;
            source.stat->under = base_tiles[i];

            if (moved_tile.element != ZZT_PLAYER)
                moved_tile.bg = base_tiles[i].bg;
        }

        Bzzt_Board_Set_Tile(b, ring[i].x, ring[i].y, moved_tile);
    }
}

static void update_conveyor_glyph(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat, bool clockwise)
{
    static const uint8_t cw_seq[] = {179, '/', 196, '\\'};
    static const uint8_t ccw_seq[] = {179, '\\', 196, '/'};

    if (!w || !w->timer || !b || !stat)
        return;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);
    int phase = (stat->cycle > 0) ? ((w->timer->current_tick / stat->cycle) % 4) : 0;
    tile.glyph = clockwise ? cw_seq[phase] : ccw_seq[phase];
    Bzzt_Board_Set_Tile(b, stat->x, stat->y, tile);
}

static void zzt_conveyor_tick(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat, bool clockwise)
{
    if (!w || !b || !stat)
        return;

    ConveyorRingCell ring[8];
    int targets[8];

    update_conveyor_glyph(w, b, stat, clockwise);
    build_conveyor_ring(b, stat, ring);
    compute_conveyor_targets(ring, clockwise, targets);
    apply_conveyor_targets(b, ring, targets);
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
                    spawn_blast = (t.element == ZZT_EMPTY);
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
            else if (t.element == ZZT_EMPTY)
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
                Bzzt_Board_Set_Tile(b, tx, ty, empty_tile);
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

    case ZZT_DUPLICATOR:
        zzt_duplicator_tick(ui, w, b, stat);
        break;

    case ZZT_CWCONV:
        zzt_conveyor_tick(w, b, stat, true);
        break;

    case ZZT_CCWCONV:
        zzt_conveyor_tick(w, b, stat, false);
        break;

    case ZZT_TRANSPORTER:
        update_transporter_glyph(w, b, stat);
        break;

    case ZZT_BOMB:
        zzt_bomb_tick(ui, w, b, stat);
        break;

    case ZZT_BLINK:
        zzt_blink_wall_tick(ui, w, b, stat);
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
    bool head_uses_transporter = false;

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
        else if (t.element == ZZT_TRANSPORTER)
        {
            Bzzt_Stat *transporter = Bzzt_Board_Get_Stat_At(b, t.x, t.y);
            int transport_x = 0;
            int transport_y = 0;
            if (transporter &&
                transporter_faces_direction(transporter, direction) &&
                transporter_resolve_exit(b, t.x, t.y, direction, &transport_x, &transport_y))
            {
                can_push = true;
                head_uses_transporter = true;
                break;
            }
            break;
        }
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
        if (i == 0 && head_uses_transporter)
        {
            if (stat)
                transporter_try_move_stat(b, stat, dest_x, dest_y, direction);
            else
                transporter_try_move_tile(b, src, dest_x, dest_y, direction);
        }
        else if (stat)
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
    Direction move_dir = (stat->step_x > 0) ? DIR_RIGHT : (stat->step_x < 0) ? DIR_LEFT
                                                 : (stat->step_y > 0)         ? DIR_DOWN
                                                 : (stat->step_y < 0)         ? DIR_UP
                                                                              : DIR_NONE;
    if (next_tile.element == ZZT_TRANSPORTER &&
        move_dir != DIR_NONE &&
        transporter_try_move_stat(b, stat, next_x, next_y, move_dir))
        return;

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
