#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "color.h"

typedef struct
{
    uint8_t element_id;
    uint8_t default_glyph;
    uint8_t default_fg_idx;  // EGA color index (0-15)
    uint8_t default_bg_idx;  // EGA color index (0-7)
    int16_t default_cycle;   // -1 means no stat required
    bool requires_stat;      // Whether this element needs a stat entry
    uint8_t default_data[3]; // Default param data values
} ZZT_Element_Defaults;

// Based on ZZT's internal element table
static const ZZT_Element_Defaults zzt_element_defaults_table[] = {
    // ID, Glyph, FG, BG, Cycle, NeedsStat, Data[0,1,2]
    {ZZT_EMPTY, ' ', 15, 0, -1, false, {0, 0, 0}},
    {ZZT_EDGE, 219, 15, 0, -1, false, {0, 0, 0}},         // Board edge (special)
    {ZZT_MESSAGETIMER, 'T', 15, 0, -1, false, {0, 0, 0}}, // Internal timer
    {ZZT_MONITOR, 'M', 15, 0, -1, false, {0, 0, 0}},      // Internal monitor
    {ZZT_PLAYER, 2, 15, 1, 1, true, {0, 0, 0}},           // ☻, white on blue
    {ZZT_AMMO, 132, 3, 0, -1, false, {0, 0, 0}},          // ä, cyan
    {ZZT_TORCH, 157, 14, 0, -1, false, {0, 0, 0}},        // Ø, yellow
    {ZZT_GEM, 4, 0, 0, -1, false, {0, 0, 0}},             // ♦, color varies
    {ZZT_KEY, 12, 0, 0, -1, false, {0, 0, 0}},            // ♀, color varies
    {ZZT_DOOR, 10, 0, 0, -1, false, {0, 0, 0}},           // ◙, color  varies (bg)
    {ZZT_SCROLL, 232, 15, 0, -1, true, {0, 0, 0}},        // Φ, white
    {ZZT_PASSAGE, 240, 0, 0, -1, true, {0, 0, 0}},        // ≡, color varies
    {ZZT_DUPLICATOR, 250, 15, 0, 2, true, {0, 0, 0}},     // ·, white, cycle 2
    {ZZT_BOMB, 11, 15, 0, 6, true, {0, 0, 0}},            // ♂, white, cycle 6
    {ZZT_ENERGIZER, 127, 15, 0, -1, false, {0, 0, 0}},    // ⌂, white
    {ZZT_STAR, '/', 15, 0, 1, true, {0, 0, 100}},         // Star (data[2]=100 cycles)
    {ZZT_CWCONV, 179, 15, 0, 3, true, {1, 0, 0}},         // │, clockwise
    {ZZT_CCWCONV, '\\', 15, 0, 3, true, {-1, 0, 0}},      // \, counter-clockwise
    {ZZT_BULLET, 248, 15, 0, 1, true, {0, 0, 0}},         // °, white
    {ZZT_WATER, 176, 9, 0, -1, false, {0, 0, 0}},         // ░, light blue (blinking)
    {ZZT_FOREST, 176, 2, 0, -1, false, {0, 0, 0}},        // ░, green
    {ZZT_SOLID, 219, 7, 0, -1, false, {0, 0, 0}},         // █, gray
    {ZZT_NORMAL, 178, 7, 0, -1, false, {0, 0, 0}},        // ▓, gray
    {ZZT_BREAKABLE, 177, 14, 0, -1, false, {0, 0, 0}},    // ▒, yellow
    {ZZT_BOULDER, 254, 14, 0, -1, false, {0, 0, 0}},      // ■, yellow (no stat unless moving)
    {ZZT_NSSLIDER, 18, 15, 0, -1, false, {0, 0, 0}},      // ↕, white
    {ZZT_EWSLIDER, 29, 15, 0, -1, false, {0, 0, 0}},      // ↔, white
    {ZZT_FAKE, 178, 7, 0, -1, false, {0, 0, 0}},          // ▓, gray (looks like normal)
    {ZZT_INVISIBLE, 176, 0, 0, -1, false, {0, 0, 0}},     // ░, black (invisible)
    {ZZT_BLINK, 206, 0, 0, 1, true, {1, 0, 0}},           // ╬, color varies, data[0]=1 (starting phase)
    {ZZT_TRANSPORTER, 0, 0, 0, 2, true, {0, 0, 0}},       // <^>v (varies), cycle 2
    {ZZT_LINE, 0, 0, 0, -1, false, {0, 0, 0}},            // Line (glyphvaries by neighbors)
    {ZZT_RICOCHET, '*', 10, 0, -1, false, {0, 0, 0}},     // *, light green
    {ZZT_BLINKHORIZ, 205, 0, 0, -1, false, {0, 0, 0}},    // ═, (ray - no cycle)
    {ZZT_BEAR, 153, 6, 0, 3, true, {0, 0, 0}},            // ○, brown, cycle 3
    {ZZT_RUFFIAN, 153, 13, 0, 1, true, {0, 0, 0}},        // ○, magenta,cycle 1
    {ZZT_OBJECT, 2, 15, 0, 3, true, {1, 0, 0}},           // ☻ (default), data[0]=glyph
    {ZZT_SLIME, '*', 0, 0, 3, true, {0, 0, 0}},           // *, color varies (breaking through)
    {ZZT_SHARK, '^', 7, 0, 3, true, {0, 0, 0}},           // ^, gray, cycle 3
    {ZZT_SPINNINGGUN, 24, 0, 0, 2, true, {0, 0, 0}},      // ↑, color varies, cycle 2
    {ZZT_PUSHER, 31, 0, 0, 4, true, {0, 0, 0}},           // ▼ (default), cycle 4
    {ZZT_LION, 234, 12, 0, 2, true, {0, 0, 0}},           // Ω, red, cycle 2
    {ZZT_TIGER, 227, 11, 0, 2, true, {0, 0, 0}},          // π, cyan, cycle 2
    {ZZT_BLINKVERT, 186, 0, 0, -1, false, {0, 0, 0}},     // ║, (ray - no cycle)
    {ZZT_CENTHEAD, 233, 9, 0, 2, true, {0, 0, 0}},        // Θ (head), light blue, cycle 2
    {ZZT_CENTBODY, 'O', 9, 0, 2, true, {0, 0, 0}},        // O (segment), light blue, cycle 2
};

#define ZZT_ELEMENT_DEFAULTS_COUNT (sizeof(zzt_element_defaults_table) / sizeof(ZZT_Element_Defaults))

// Helper function to get defaults for an element type
static inline const ZZT_Element_Defaults *zzt_get_element_defaults(uint8_t element_id)
  {
      for (size_t i = 0; i < ZZT_ELEMENT_DEFAULTS_COUNT; i++)
      {
          if (zzt_element_defaults_table[i].element_id == element_id)
              return &zzt_element_defaults_table[i];
      }
      return NULL; // Element not found
  }
