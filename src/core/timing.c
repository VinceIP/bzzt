#include "timing.h"
#include "bzzt.h"

void Bzzt_Timer_Tick(Bzzt_Timer *t)
{
    if (!t || t->paused)
        return;

    if (t->current_tick + 1 > 420 || t->current_tick == 0)
        t->current_tick = 1;
    else
        t->current_tick++;
}

void Bzzt_Timer_Run_Frame(Bzzt_World *w, double frame_ms)
{
    if (!w || !w->timer)
        return;

    w->timer->accumulator_ms += frame_ms;
    while (w->timer->accumulator_ms >= w->timer->tick_duration_ms)
    {
        w->timer->accumulator_ms -= Bzzt_Timer_Run_Tick(w);
        if (w->paused)
            break;
    }
}

double Bzzt_Timer_Run_Tick(Bzzt_World *w)
{
    if (!w || !w->timer)
        return 0.0;

    Bzzt_Board *current_board = w->boards[w->boards_current];
    if (!current_board)
        return w->timer->tick_duration_ms;

    for (int i = 0; i < current_board->stat_count; ++i)
    {
        Bzzt_Stat *stat = current_board->stats[i];
        if (stat)
        {
            stat->prev_x = stat->x;
            stat->prev_y = stat->y;
        }
    }

    int initial_count = current_board->stat_count;
    for (int i = 0; i < current_board->stat_count; ++i)
    {
        Bzzt_Stat *stat = current_board->stats[i];
        if (stat)
        {
            int count_before = current_board->stat_count;
            Bzzt_Stat_Update(w, stat, i);

            if (current_board->stat_count < count_before)
            {
                i--;
            }
        }
    }

    Bzzt_Timer_Tick(w->timer);
    return w->timer->tick_duration_ms;
}
