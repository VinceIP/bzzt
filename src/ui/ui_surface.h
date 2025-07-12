/**
 * @file ui_surface.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <stdint.h>
#include "color.h"

typedef struct UIElement UIElement;
struct Cell;
typedef struct Cell Cell;
typedef struct UISurface
{
    bool visible;
    int x, y, z;
    int w, h;
    Cell *cells;
    int cell_count;
    UIElement **elements;
    int elements_count, elements_cap;
} UISurface;

void UISurface_Destroy(UISurface *);