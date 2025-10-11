#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Bzzt_World Bzzt_World;

typedef struct Bzzt_Timer
{
    double tick_duration_ms;
    double accumulator_ms;
    uint16_t current_tick;
    int current_stat_index;
    bool paused;
} Bzzt_Timer;

void Bzzt_Timer_Tick(Bzzt_Timer *t);

void Bzzt_Timer_Run_Frame(Bzzt_World *w, double frame_ms);

double Bzzt_Timer_Run_Tick(Bzzt_World *w);