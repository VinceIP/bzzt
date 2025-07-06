#pragma once
#include <cstdint>

typedef enum {
    DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} Direction;

typedef struct
{
    int id;
    int x, y;
    Direction dir;
    uint8_t glyph, fg_color, bg_color;
} Object;