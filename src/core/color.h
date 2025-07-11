#pragma once
#include <stdint.h>
typedef struct
{
    uint8_t r, g, b;
} Color_Bzzt;

typedef enum
{
    BZ_BLACK = 0,
    BZ_BLUE,
    BZ_GREEN,
    BZ_CYAN,
    BZ_RED,
    BZ_MAGENTA,
    BZ_BROWN,
    BZ_LIGHT_GRAY,
    BZ_DARK_GRAY,
    BZ_LIGHT_BLUE,
    BZ_LIGHT_GREEN,
    BZ_LIGHT_CYAN,
    BZ_LIGHT_RED,
    BZ_LIGHT_MAGENTA,
    BZ_YELLOW,
    BZ_WHITE
} BzztColor;

static const Color_Bzzt COLOR_BLACK = {0, 0, 0};
static const Color_Bzzt COLOR_BLUE = {0, 0, 170};
static const Color_Bzzt COLOR_GREEN = {0, 170, 0};
static const Color_Bzzt COLOR_CYAN = {0, 170, 170};
static const Color_Bzzt COLOR_RED = {170, 0, 0};
static const Color_Bzzt COLOR_MAGENTA = {170, 0, 170};
static const Color_Bzzt COLOR_BROWN = {170, 85, 0};
static const Color_Bzzt COLOR_LIGHT_GRAY = {170, 170, 170};
static const Color_Bzzt COLOR_DARK_GRAY = {85, 85, 85};
static const Color_Bzzt COLOR_LIGHT_BLUE = {85, 85, 255};
static const Color_Bzzt COLOR_LIGHT_GREEN = {85, 255, 85};
static const Color_Bzzt COLOR_LIGHT_CYAN = {85, 255, 255};
static const Color_Bzzt COLOR_LIGHT_RED = {255, 85, 85};
static const Color_Bzzt COLOR_LIGHT_MAGENTA = {255, 85, 255};
static const Color_Bzzt COLOR_YELLOW = {255, 255, 85};
static const Color_Bzzt COLOR_WHITE = {255, 255, 255};

static const Color_Bzzt BZZT_PALETTE[16] = {
    COLOR_BLACK,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_CYAN,
    COLOR_RED,
    COLOR_MAGENTA,
    COLOR_BROWN,
    COLOR_LIGHT_GRAY,
    COLOR_DARK_GRAY,
    COLOR_LIGHT_BLUE,
    COLOR_LIGHT_GREEN,
    COLOR_LIGHT_CYAN,
    COLOR_LIGHT_RED,
    COLOR_LIGHT_MAGENTA,
    COLOR_YELLOW,
    COLOR_WHITE};

static inline Color_Bzzt bzzt_get_color(uint8_t idx)
{
    return BZZT_PALETTE[idx & 0x0F];
}