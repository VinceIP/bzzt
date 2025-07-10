#pragma once
#include <stdint.h>
#include "color.h"

typedef enum
{
    DIR_NONE,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef struct
{
    int id;
    int x, y;
    Direction dir;
    uint8_t glyph;
    Color_bzzt fg_color, bg_color;
} Object;

Object *Object_Create(uint8_t glyph, Color_bzzt fg_color, Color_bzzt bg_color, int x, int y);
void Object_Destroy(Object *o);
