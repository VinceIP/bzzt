#include "object.h"

Object *object_create(uint8_t glyph, Color_bzzt fg_color, Color_bzzt bg_color, int x, int y){
    Object o = {
        .glyph = 2,
        .bg_color = COLOR_BLUE,
        .fg_color = COLOR_WHITE,
        .x = x,
        .y = y
    };
}