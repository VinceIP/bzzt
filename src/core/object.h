#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "color.h"
#include "cell.h"

typedef enum
{
    DIR_NONE,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef struct Object
{
    int id;
    int x, y;
    Direction dir;
    Cell cell;
} Object;

Object *Object_Create(uint8_t glyph, Color_Bzzt fg, Color_Bzzt bg, int x, int y);
void Object_Destroy(Object *o);
