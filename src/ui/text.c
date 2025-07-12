#include "text.h"
#include "ui.h"
#include "color.h"
#include "code_page_lut.h"
#include "object.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void UIText_WriteRaw(UISurface *surface, const char *utf8, int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int wrapWidth)
{
    int maxWidth = wrap ? (wrapWidth > 0 ? wrapWidth : surface->width) : surface->width; // Set maxWidth to surface width if wrap is false or wrapWidth <= 0
    int maxHeight = surface->height;
    int cursX = x;
    int cursY = y;
    int len = strlen(utf8);

    for (int i = 0; i < len; ++i)
    {
        // Handle cursor movement
        uint16_t c = (unsigned char)utf8[i];
        if (c == '\n')
        {
            cursX = x;
            cursY += 1;
        }
        if (cursX >= x + maxWidth)
        {
            if (wrap)
            {
                cursX = x;
                cursY += 1;
            }
            else
            {
                break; // Stop drawing if out of bounds
            }
        }
        if (cursY >= maxHeight)
        {
            break; // Stop drawing when out of vertical space
        }
        ////
        uint8_t glyph = unicode_to_cp437(c);  // Get a CP437 glyph
        int index = cursY * maxWidth + cursX; // Calculate flat array index;
        Cell *cell = &surface->cells[index];  // Get a cell and set its properties
        cell->glyph = glyph;
        cell->fg = fg;
        cell->bg = bg;

        cursX += 1;
    }
}