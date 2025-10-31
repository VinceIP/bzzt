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

static const char *BZZT_COLOR_NAMES[] = {
    "black", "dark blue", "dark green", "dark cyan", "dark red", "dark purple", "brown", "gray", "dark gray", "blue", "green", "cyan", "red", "purple", "yellow", "white", "transparent"};

static inline Color_Bzzt bzzt_get_color(uint8_t idx)
{
    return BZZT_PALETTE[idx & 0x0F];
}

static inline bool bzzt_color_equals(Color_Bzzt a, Color_Bzzt b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

// Get the name of a color as a string from color index
static inline const char *bzzt_color_name(uint8_t idx)
{
    if (idx >= 17)
        return "unknown";
    return BZZT_COLOR_NAMES[idx];
}

// Get the name of a color from BzztColor enum value
static inline const char *bzzt_color_name_from_enum(int color_enum)
{
    return bzzt_color_name((uint8_t)color_enum);
}

static inline int bzzt_color_to_index(Color_Bzzt color)
{
    // Search through the palette to find matching color
    for (int i = 0; i < 17; i++)
    {
        if (bzzt_color_equals(color, BZZT_PALETTE[i]))
            return i;
    }
    return -1; // Not found
}

// Get the name of a color directly from a Color_Bzzt value
static inline const char *bzzt_color_name_from_color(Color_Bzzt color)
{
    int idx = bzzt_color_to_index(color);
    if (idx < 0)
        return "unknown";
    return bzzt_color_name((uint8_t)idx);
}
