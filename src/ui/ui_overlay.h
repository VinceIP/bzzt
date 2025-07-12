/**
 * @file overlay.h
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

typedef struct UIElement UIElement;

/**
 * @brief A sub surface to be overlayed on a UISurface and toggled when needed.
 * Can hold generic UI elements such as text fields, buttons, etc
 *
 */
typedef struct UIOverlay
{
    UIElement **elements;
    int x, y, z;
    bool visible;
} UIOverlay;

UIOverlay *UIOverlay_Create();
void UIOverlay_Update(UIOverlay *);
void UIOverlay_Destroy(UIOverlay *);

/**
 * @brief Print formatted text to a UIOverlay.
 *
 * @param ov Target overlay.
 * @param fg fg color
 * @param bg bg color
 * @param wrap Enable to ensure text wrapping
 * @param fmt Text string with/without format specifiers
 * @param ... Additional args to be formatted
 */
void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *fmt, ...);