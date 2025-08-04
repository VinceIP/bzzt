#include "ui.h"
#include "bzzt.h"
#include "color.h"
#include "code_page_lut.h"
#include "debugger.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

bool UISurface_DrawText(UISurface *surface, const char *utf8, int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int wrapWidth)
{
    // int maxWidth = wrap ? (wrapWidth > 0 ? wrapWidth : surface->properties.w) : surface->properties.w; // Set maxWidth to surface width if wrap is false or wrapWidth <= 0
    int maxHeight = surface->properties.h;
    int cursX = x;
    int cursY = y;
    Color_Bzzt cur_fg = fg;
    Color_Bzzt cur_bg = bg;
    int len = strlen(utf8);
    for (int i = 0; i < len;)
    {
        if (utf8[i] == '\\')
        {
            Debug_Printf(LOG_UI, "Saw an escape char.");
            char next = utf8[i + 1];
            if (next == 'n')
            {
                Debug_Printf(LOG_UI, "Saw a newline.");
                cursX = x;
                cursY += 1;
                i += 2;
                continue;
            }
            else if (next == 'f' || next == 'b')
            {
                bool set_fg = (next == 'f');
                Debug_Printf(LOG_UI, "Saw a fg change.");

                i += 2;
                int val = 0;
                while (utf8[i] && isdigit((unsigned char)utf8[i]))
                {
                    val = val * 10 + (utf8[i] - '0');
                    i++;
                }
                if (set_fg)
                    cur_fg = bzzt_get_color((uint8_t)val);
                else
                    cur_bg = bzzt_get_color((uint8_t)val);
                continue;
            }
        }
        else if (utf8[i] == '\n')
        {
            cursX = x;
            cursY += 1;
            i++;
            continue;
        }

        if (cursX >= x + surface->properties.w)
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

        uint16_t c = (unsigned char)utf8[i];
        uint8_t glyph = unicode_to_cp437(c);               // Get a CP437 glyph
        int index = cursY * surface->properties.w + cursX; // Calculate flat array index
        Bzzt_Cell *cell = &surface->cells[index];          // Get a cell and set its properties
        cell->glyph = glyph;
        cell->fg = cur_fg;
        cell->bg = cur_bg;
        cell->visible = true;

        cursX += 1;
        i++;
    }
    return false;
}