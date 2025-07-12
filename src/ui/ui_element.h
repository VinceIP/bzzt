/**
 * @file ui_element.h
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

typedef enum
{
    UI_ELEM_NONE,
    UI_ELEM_TEXT,
    UI_ELEM_BUTTON,
    UI_ELEM_INPUT
} ElementType;

typedef struct UIElement
{
    ElementType type;
    bool visible;
    int x, y;
    void (*update)(struct UIElement *);
} UIElement;

typedef struct UIText
{
    UIElement base;
    const char *(*textCallback)(void *ud);
    void *ud;
    Color_Bzzt fg, bg;
    bool wrap;
} UIText;

typedef void (*UIButtonAction)(void *ud);

typedef struct UIButton
{
    UIElement base;
    UIText *label;
    UIButtonAction onClick;
    void *ud;
} UIButton;

UIElement *UIElement_Create();
void UIElement_Update(struct UIElement *);
void UIElement_Destroy(UIElement *e);

UIText *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg,
                      const char *(*cb)(void *), void *ud);
UIButton *UIButton_Create(int x, int y, const char *caption,
                          UIButtonAction cb, void *ud);
