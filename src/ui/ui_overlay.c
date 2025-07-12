/**
 * @file overlay.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-12
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui_overlay.h"
#include "color.h"
#include "text.h"
#include "ui.h"
#include "ui_surface.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *fmt, ...)
{
    char buffer[256]; // Increased buffer size for safety
    va_list args;     // Handle args
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args); // Format text using any args
    va_end(args);

    int absX = ov->x;
    int absY = ov->y;
    UISurface *s = ov->surface;

    UIText_WriteRaw(ov->surface, buffer, absX, absY, fg, bg, wrap, s->w);
}