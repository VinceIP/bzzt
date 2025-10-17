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

static int get_key_index_from_color(Color_Bzzt color)
{
    // Map bzzt color to key index
    if (bzzt_color_equals(color, bzzt_get_color(BZ_LIGHT_BLUE)))
        return ZZT_KEY_BLUE;
    else if (bzzt_color_equals(color, bzzt_get_color(BZ_LIGHT_GREEN)))
        return ZZT_KEY_GREEN;
    else if (bzzt_color_equals(color, bzzt_get_color(BZ_LIGHT_CYAN)))
        return ZZT_KEY_CYAN;
    else if (bzzt_color_equals(color, bzzt_get_color(BZ_LIGHT_RED)))
        return ZZT_KEY_RED;
    else if (bzzt_color_equals(color, bzzt_get_color(BZ_LIGHT_MAGENTA)))
        return ZZT_KEY_PURPLE;
    else if (bzzt_color_equals(color, bzzt_get_color(BZ_YELLOW)))
        return ZZT_KEY_YELLOW;
    else if (bzzt_color_equals(color, bzzt_get_color(BZ_WHITE)))
        return ZZT_KEY_WHITE;

    return -1; // Invalid color
}

static int get_key_index_from_door_color(Color_Bzzt door_color)
{
    if (bzzt_color_equals(door_color, bzzt_get_color(BZ_BLUE)))
        return ZZT_KEY_BLUE;
    else if (bzzt_color_equals(door_color, bzzt_get_color(BZ_GREEN)))
        return ZZT_KEY_GREEN;
    else if (bzzt_color_equals(door_color, bzzt_get_color(BZ_CYAN)))
        return ZZT_KEY_CYAN;
    else if (bzzt_color_equals(door_color, bzzt_get_color(BZ_RED)))
        return ZZT_KEY_RED;
    else if (bzzt_color_equals(door_color, bzzt_get_color(BZ_MAGENTA)))
        return ZZT_KEY_PURPLE;
    else if (bzzt_color_equals(door_color, bzzt_get_color(BZ_BROWN)))
        return ZZT_KEY_YELLOW; // Brown door -> Yellow key
    else if (bzzt_color_equals(door_color,
                               bzzt_get_color(BZ_LIGHT_GRAY)))
        return ZZT_KEY_WHITE; // Light gray door -> White key

    return -1; // Invalid door color
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

static void handle_bullet_collision(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *bullet, int collision_x, int collision_y)
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
        // tbd
        break;
    case ZZT_OBJECT:
        // tbd
        break;
    case ZZT_BULLET: // bullets destroy each other
        // tbd
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
        // tbd player collision
        break;
    default: // Hitting any other element kills the bullet
        Bzzt_Board_Stat_Die(b, bullet);
        break;
    }
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

static bool handle_item_pickup(Bzzt_World *w, Bzzt_Board *b, int x, int y, Bzzt_Tile item)
{
    Bzzt_Tile empty = {0};
    Bzzt_Stat *player = b->stats[0];

    switch (item.element)
    {
    case ZZT_AMMO:
        w->ammo += 5;
        Bzzt_Board_Set_Tile(b, x, y, empty);
        return true;
    case ZZT_GEM:
        w->gems++;
        w->score += 10;
        Bzzt_Board_Set_Tile(b, x, y, empty);
        return true;
    case ZZT_TORCH:
        w->torches++;
        Bzzt_Board_Set_Tile(b, x, y, empty);
        return true;
    case ZZT_ENERGIZER:
        // do energize
        Bzzt_Board_Set_Tile(b, x, y, empty);
        return true;
    case ZZT_KEY:
    {
        Color_Bzzt key_color = item.fg;
        int key_idx = get_key_index_from_color(key_color);
        Debug_Log(LOG_LEVEL_DEBUG, LOG_ENGINE, "key idx: %d", key_idx);
        if (key_idx < 0 || key_idx >= 7)
            return false;
        if (w->keys[key_idx] != 0)
            return false; // Already have key
        w->keys[key_idx] = 1;
        Bzzt_Board_Set_Tile(b, x, y, empty);
        return true;
    }
    case ZZT_SCROLL:
        return true;
    default:
        return false;
    }
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

    if (Bzzt_World_Switch_Board_To(w, target_board_idx, dest_x, dest_y))
    {
        puts("doing pause");
        Bzzt_World_Set_Pause(w, true);
    }
    return true;
}

// returns the type of thing the player touched
static uint8_t handle_player_touch(Bzzt_World *w, Bzzt_Tile tile, int x, int y)
{
    if (!w)
        return false;

    Bzzt_Board *board = w->boards[w->boards_current];
    Bzzt_Stat *player = board->stats[0];
    Bzzt_Tile player_tile = Bzzt_Board_Get_Tile(board, player->x, player->y);
    const Bzzt_Tile empty_tile = {0};

    Interaction_Type type = Bzzt_Tile_Get_Interaction_Type(tile);
    const char *type_name = Bzzt_Tile_Get_Type_Name(tile);
    Debug_Printf(LOG_LEVEL_DEBUG, "Player touched %s at (%d, %d).", type_name, x, y);

    // Handle solid, interactable tiles here
    switch (tile.element)
    {
    case ZZT_PASSAGE:
        handle_passage_touch(w, x, y);
        break;
    case ZZT_INVISIBLE:
        if (!tile.visible)
        {
            tile.visible = true;
            Bzzt_Board_Set_Tile(board, x, y, tile);
        }
        break;
    case ZZT_FOREST:
        Bzzt_Board_Set_Tile(board, x, y, empty_tile);
        Debug_Printf(LOG_WORLD, "Moved through the forest.");
        break;
    case ZZT_DOOR:
        Color_Bzzt door_color = tile.bg;
        int key_idx = get_key_index_from_door_color(door_color);
        if (key_idx < 0 || key_idx >= 7)
            return false;
        if (w->keys[key_idx] == 0) // If player doesn't have this key
            return false;
        w->keys[key_idx] = 0; // success, consume key
        Bzzt_Board_Set_Tile(board, x, y, empty_tile);
        break;
    default:
        break;
    }

    return tile.element;
}

static void handle_player_move(Bzzt_World *w)
{
    if (!w || !w->current_input)
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
        handle_board_edge_move(w, new_x, new_y);
        return;
    }

    uint8_t element_type_touched = 0;

    Bzzt_Tile target_tile = Bzzt_Board_Get_Tile(current_board, new_x,
                                                new_y);
    if (Bzzt_Tile_Is_Walkable(target_tile))
    {
        Bzzt_Board_Move_Stat_To(current_board, stat, new_x, new_y);
    }
    else if (handle_item_pickup(w, current_board, new_x, new_y,
                                target_tile))
        Bzzt_Board_Move_Stat_To(current_board, stat, new_x, new_y);
    else
        element_type_touched = handle_player_touch(w, target_tile, new_x,
                                                   new_y);

    if (dx != 0)
        stat->step_x = (dx > 0) ? 1 : -1;
    if (dy != 0)
        stat->step_y = (dy > 0) ? 1 : -1;

    if (w->paused && element_type_touched != ZZT_PASSAGE)
        Bzzt_World_Set_Pause(w, false);
}

static void handle_player_shoot(Bzzt_World *w, Bzzt_Stat *player_stat)
{
    if (!w || !player_stat)
        return;

    InputState *in = w->current_input;
    if (!in)
        return;

    if (w->ammo <= 0)
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

static void stat_act(Bzzt_World *w, Bzzt_Board *b, Bzzt_Stat *stat)
{
    if (!w || !b || !stat)
        return;

    Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);
    uint8_t stat_type = tile.element;

    switch (stat_type)
    {
    case ZZT_PLAYER:
        handle_player_shoot(w, stat);
        if (!w->current_input->SHIFT_held)
            handle_player_move(w);
        break;

    case ZZT_BULLET:
        int next_x = stat->x + stat->step_x;
        int next_y = stat->y + stat->step_y;

        if (!Bzzt_Board_Is_In_Bounds(b, next_x, next_y))
        {
            Bzzt_Board_Stat_Die(b, stat);
            break;
        }

        Bzzt_Tile next_tile = Bzzt_Board_Get_Tile(b, next_x, next_y);
        if (next_tile.element != ZZT_EMPTY && next_tile.element != ZZT_FAKE && next_tile.element != ZZT_WATER)
            handle_bullet_collision(w, b, stat, next_x, next_y);
        else
            Bzzt_Board_Move_Stat_To(b, stat, next_x, next_y);
        break;
    case ZZT_SPINNINGGUN:
        // tbd - add star firing
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

            if (fire_intelligently && player)
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
                    if (Bzzt_Stat_Is_Blocked(b, stat, fire_dir))
                        fire_dir = DIR_NONE;
                }

                // Try horizontal shot if player is within 2 tiles vertically
                if (fire_dir == DIR_NONE && abs_dy <= 2)
                {
                    fire_dir = (dx > 0) ? DIR_RIGHT : DIR_LEFT;
                    // Check if horizontal path is clear
                    if (Bzzt_Stat_Is_Blocked(b, stat, fire_dir))
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
        break;
    default:
        break;
    }
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

void Bzzt_Stat_Update(Bzzt_World *w, Bzzt_Stat *stat, int stat_idx)
{
    if (!w || !stat)
        return;

    Bzzt_Board *current_board = w->boards[w->boards_current];

    if (stat_can_act(w, stat, stat_idx))
        stat_act(w, current_board, stat);
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

void Bzzt_Object_Destroy(Bzzt_Object *o)
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
        return true;
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

Bzzt_Tile Bzzt_Tile_From_ZZT_Tile(ZZTblock *block, int x, int y)
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
