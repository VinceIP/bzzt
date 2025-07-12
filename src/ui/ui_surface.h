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
#include <stdbool.h>
#include "color.h"

typedef struct UIOverlay UIOverlay;
struct Cell;
typedef struct Cell Cell;
/**
 * @brief A drawable UI surface. Can hold a glyph bitmap.
 * Can hold multiple UI overlay containers to be drawn on top of this surface.
 *
 */
typedef struct UISurface
{
    bool visible;
    int x, y, z;
    int w, h;
    Cell *cells;
    int cell_count;
    UIOverlay **overlays;
    int elements_count, elements_cap;
} UISurface;

UISurface *UISurface_Create();
void UISurface_Update();
void UISurface_Destroy(UISurface *);