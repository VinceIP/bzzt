/**
 * @file text.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "color.h"
#include <stdbool.h>

typedef struct UISurface UISurface;

void UIText_WriteRaw(UISurface *surface, const char *utf8, int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int wrapWidth);