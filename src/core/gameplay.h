#pragma once

#include <stdint.h>
#include "bzzt.h"

typedef struct UI UI;

typedef struct GameplayContext
{
    UI *ui;
    Bzzt_World *world;
    Bzzt_Board *board;
} GameplayContext;

GameplayContext GameplayContext_Make(UI *ui, Bzzt_World *world, Bzzt_Board *board);
void Gameplay_Award_Score_For_Element(Bzzt_World *world, uint8_t element);
Direction Gameplay_Seek_Direction_To_Player(Bzzt_World *world, Bzzt_Board *board, Bzzt_Stat *stat);
