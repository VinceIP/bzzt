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
typedef struct ZZTBlock ZZTBlock;

void Renderer_Draw_Board(Renderer *r, const Board *b);
void Renderer_Draw_ZZT_Board(Renderer *r, ZZTblock *b);