/**
 * @file board_renderer.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "renderer.h"

typedef struct Board Board;
typedef struct ZZTblock ZZTblock;
typedef struct ZZTworld ZZTworld;

void Renderer_Draw_Board(Renderer *, const Board *);
void Renderer_Draw_ZZT_Board(Renderer *r, ZZTworld *w, ZZTblock *b);