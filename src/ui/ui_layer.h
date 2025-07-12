/**
 * @file ui_layer.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <stdbool.h>

typedef struct UISurface UISurface;

/**
 * @brief A UI layer that holds multiple UI drawing surfaces.
 *
 */
typedef struct UILayer
{
    bool visible;
    int z;
    UISurface **surfaces;
    int surface_count, surface_cap;
} UILayer;

UILayer *UILayer_Create();
UISurface *UILayer_Add_Surface(UILayer *, int w, int h, int x, int y);
void UILayer_Update(UILayer *);
void UILayer_Destroy(UILayer *);