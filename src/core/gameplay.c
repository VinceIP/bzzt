#include "gameplay.h"

static Direction direction_from_stat_step(Bzzt_Stat *stat)
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

GameplayContext GameplayContext_Make(UI *ui, Bzzt_World *world, Bzzt_Board *board)
{
    GameplayContext ctx = {
        .ui = ui,
        .world = world,
        .board = board,
    };
    return ctx;
}

void Gameplay_Award_Score_For_Element(Bzzt_World *world, uint8_t element)
{
    if (!world)
        return;

    switch (element)
    {
    case ZZT_BEAR:
    case ZZT_LION:
        Bzzt_World_Inc_Score(world, 1);
        break;
    case ZZT_RUFFIAN:
    case ZZT_TIGER:
        Bzzt_World_Inc_Score(world, 2);
        break;
    default:
        break;
    }
}

Direction Gameplay_Seek_Direction_To_Player(Bzzt_World *world, Bzzt_Board *board, Bzzt_Stat *stat)
{
    if (!world || !board || !stat || board->stat_count <= 0 || !board->stats[0])
        return direction_from_stat_step(stat);

    Bzzt_Stat *player = board->stats[0];
    int dx = player->x - stat->x;
    int dy = player->y - stat->y;

    if (Bzzt_World_Is_Energized(world))
    {
        dx = -dx;
        dy = -dy;
    }

    int abs_dx = dx < 0 ? -dx : dx;
    int abs_dy = dy < 0 ? -dy : dy;
    Direction current_dir = direction_from_stat_step(stat);

    if (abs_dx == abs_dy)
    {
        if ((current_dir == DIR_LEFT || current_dir == DIR_RIGHT) && dx != 0)
            return dx > 0 ? DIR_RIGHT : DIR_LEFT;
        if ((current_dir == DIR_UP || current_dir == DIR_DOWN) && dy != 0)
            return dy > 0 ? DIR_DOWN : DIR_UP;
    }

    if (abs_dx > abs_dy && dx != 0)
        return dx > 0 ? DIR_RIGHT : DIR_LEFT;
    if (dy != 0)
        return dy > 0 ? DIR_DOWN : DIR_UP;
    if (dx != 0)
        return dx > 0 ? DIR_RIGHT : DIR_LEFT;
    return current_dir;
}
