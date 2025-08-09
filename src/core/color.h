/**
 * @file color.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-08-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

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
    BZ_WHITE,
    BZ_TRANSPARENT
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
static const Color_Bzzt COLOR_TRANSPARENT = {0, 35, 0};

static const Color_Bzzt BZZT_PALETTE[17] = {
    COLOR_BLACK,         // 0
    COLOR_BLUE,          // 1
    COLOR_GREEN,         // 2
    COLOR_CYAN,          // 3
    COLOR_RED,           // 4
    COLOR_MAGENTA,       // 5
    COLOR_BROWN,         // 6
    COLOR_LIGHT_GRAY,    // 7
    COLOR_DARK_GRAY,     // 8
    COLOR_LIGHT_BLUE,    // 9
    COLOR_LIGHT_GREEN,   // 10
    COLOR_LIGHT_CYAN,    // 11
    COLOR_LIGHT_RED,     // 12
    COLOR_LIGHT_MAGENTA, // 13
    COLOR_YELLOW,        // 14
    COLOR_WHITE,         // 15
    COLOR_TRANSPARENT    // 16
};

static inline Color_Bzzt bzzt_get_color(uint8_t idx)
{
    return BZZT_PALETTE[idx & 0x0F];
}