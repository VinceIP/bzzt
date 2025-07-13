#include "text.h"
#include "ui.h"
#include "ui_surface.h"
#include "cell.h"
#include "color.h"
#include "code_page_lut.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Parse text and write it to a buffer of cells in a UI surface. Returns true if text overflows
 *
 * @param surface
 * @param utf8
 * @param x
 * @param y
 * @param fg
 * @param bg
 * @param wrap
 * @param wrapWidth
 */
bool UISurface_DrawText(UISurface *surface, const char *utf8, int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int wrapWidth)
{
    int maxWidth = wrap ? (wrapWidth > 0 ? wrapWidth : surface->w) : surface->w; // Set maxWidth to surface width if wrap is false or wrapWidth <= 0
    int maxHeight = surface->h;
    int cursX = x;
    int cursY = y;
    int len = strlen(utf8);
    for (int i = 0; i < len; ++i)
    {
        // Handle cursor movement
        uint16_t c = (unsigned char)utf8[i];
        if (c == '\n') // Newline
        {
            cursX = x;
            cursY += 1;
            continue;
        }
        if (cursX >= x + surface->w)
        {
            if (wrap)
            {
                cursX = x;
                cursY += 1;
            }
            else
            {
                return true; // Stop drawing if out of bounds
            }
        }
        if (cursY >= maxHeight)
        {
            break; // Stop drawing when out of vertical space
        }
        ////
        uint8_t glyph = unicode_to_cp437(c);    // Get a CP437 glyph
        int index = cursY * surface->w + cursX; // Calculate flat array index;
        Cell *cell = &surface->cells[index];    // Get a cell and set its properties
        cell->glyph = glyph;
        cell->fg = fg;
        cell->bg = bg;
        cell->visible = true;

        cursX += 1;
    }
    return false;
}