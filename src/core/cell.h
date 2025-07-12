#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "color.h"

typedef struct Cell
{
    bool visible;
    uint8_t glyph;
    Color_Bzzt fg, bg;
} Cell;