/**
 * @file ui_renderer.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief A simplified, explicit-positioning UI renderer with text alignment.
 * @version 0.9
 * @date 2025-08-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui_renderer.h"
#include "ui.h"
#include "code_page_lut.h"
#include "bzzt.h"
#include "debugger.h"
#include "color.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX_COLOR_CODE_DIGITS 3

// Forward declaration from the renderer, assumed to be available.
void Renderer_Draw_Cell(Renderer *r, int x, int y, unsigned char glyph, Color_Bzzt fg, Color_Bzzt bg);

// Holds metrics for a single line of text, calculated in one pass.
typedef struct
{
    const char *start; // Pointer to the first character of the line.
    const char *end;   // Pointer to the character after the last one in the line.
    int visible_width; // The width of the line in cells, excluding escape codes.
} LineMetrics;

/**
 * @brief A helper function to safely compare two Color_Bzzt structs.
 */
static inline bool is_color_equal(Color_Bzzt c1, Color_Bzzt c2)
{
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}

/**
 * @brief Gets metrics for the next line in a string in a single pass.
 */
static LineMetrics get_next_line_metrics(const char **str_ptr)
{
    LineMetrics metrics = {.start = *str_ptr, .end = *str_ptr, .visible_width = 0};
    if (!*str_ptr || **str_ptr == '\0')
        return metrics;

    while (*metrics.end != '\0' && *metrics.end != '\n' && !(*metrics.end == '\\' && *(metrics.end + 1) == 'n'))
    {
        if (*metrics.end == '\\' && (*(metrics.end + 1) == 'f' || *(metrics.end + 1) == 'b'))
        {
            metrics.end += 2; // Skip over '\f' or '\b'
            while (isdigit((unsigned char)*metrics.end))
                metrics.end++;
        }
        else
        {
            metrics.visible_width++;
            metrics.end++;
        }
    }

    // Advance the external string pointer past the current line and its newline terminator.
    *str_ptr = metrics.end;
    if (**str_ptr == '\n')
        (*str_ptr)++;
    else if (**str_ptr == '\\' && *(*str_ptr + 1) == 'n')
        *str_ptr += 2;

    return metrics;
}

/**
 * @brief Measures the maximum width and total height of a string using a single-pass metric helper.
 */
static void measure_text(const char *str, int *w, int *h)
{
    if (!str)
    {
        if (w)
            *w = 0;
        if (h)
            *h = 0;
        return;
    }

    int max_w = 0, lines = 0;
    const char *p = str;

    if (*p == '\0')
    { // Handle empty string case explicitly
        if (w)
            *w = 0;
        if (h)
            *h = 1;
        return;
    }

    while (*p)
    {
        LineMetrics metrics = get_next_line_metrics(&p);
        if (metrics.visible_width > max_w)
            max_w = metrics.visible_width;
        lines++;
    }

    if (w)
        *w = max_w;
    if (h)
        *h = lines;
}

/**
 * @brief Draws a string of text, aligning each line horizontally within a given bounding box.
 */
static void draw_text(Renderer *r, const char *str,
                      int x, int y, int maxW, int maxH,
                      Color_Bzzt fg, Color_Bzzt bg,
                      UIAlign align)
{
    if (!r || !str || !*str)
        return;

    int safe_maxW = MAX(0, maxW);
    int safe_maxH = MAX(0, maxH);

    int cursor_y = y;
    const char *p = str;

    while (*p)
    {
        if (safe_maxH > 0 && (cursor_y < y || cursor_y >= y + safe_maxH))
        {
            get_next_line_metrics(&p);
            cursor_y++;
            continue;
        }

        LineMetrics line = get_next_line_metrics(&p);

        int line_start_x = x;
        if (align == ALIGN_CENTER)
            line_start_x = x + (safe_maxW - line.visible_width) / 2;
        else if (align == ALIGN_RIGHT)
            line_start_x = x + safe_maxW - line.visible_width;

        int cursor_x = line_start_x;
        Color_Bzzt current_fg = fg;
        Color_Bzzt current_bg = bg;

        const char *line_p = line.start;
        while (line_p < line.end)
        {
            if (*line_p == '\\' && (*(line_p + 1) == 'f' || *(line_p + 1) == 'b'))
            {
                char type = *(line_p + 1);
                line_p += 2;
                char num_buf[MAX_COLOR_CODE_DIGITS + 1] = {0};
                int num_len = 0;
                while (isdigit((unsigned char)*line_p) && num_len < MAX_COLOR_CODE_DIGITS)
                {
                    num_buf[num_len++] = *line_p++;
                }
                int color_index = atoi(num_buf);

                if (color_index == BZ_TRANSPARENT)
                {
                    if (type == 'f')
                        current_fg = COLOR_TRANSPARENT;
                    else
                        current_bg = COLOR_TRANSPARENT;
                }
                else
                {
                    if (type == 'f')
                        current_fg = bzzt_get_color(color_index);
                    else
                        current_bg = bzzt_get_color(color_index);
                }
                continue;
            }

            if (safe_maxW <= 0 || (cursor_x >= x && cursor_x < x + safe_maxW))
            {
                if (cursor_x >= 0 && cursor_y >= 0)
                {
                    unsigned char cp437_glyph = unicode_to_cp437(*line_p);
                    Renderer_Draw_Cell(r, cursor_x, cursor_y, cp437_glyph, current_fg, current_bg);
                }
            }

            cursor_x++;
            line_p++;
        }
        cursor_y++;
    }
}

/**
 * @brief Draws a single UI element at a pre-calculated absolute position.
 */
static void draw_ui_element(Renderer *r, UIOverlay *overlay, UIElement *element, int abs_x, int abs_y)
{
    switch (element->type)
    {
    case UI_ELEM_TEXT:
    {
        UIElement_Text *text_elem = (UIElement_Text *)element;
        const char *str = text_elem->textCallback(text_elem->ud);
        if (str)
        {
            // For text elements, the alignment is applied to the text *within* the element's bounding box.
            draw_text(r, str, abs_x, abs_y, element->properties.w, element->properties.h, text_elem->fg, text_elem->bg, overlay->align);
        }
        break;
    }
    case UI_ELEM_BUTTON:
    {
        UIButton *button = (UIButton *)element;
        UIElement_Text *label = button->label;
        if (!label)
            break;

        if (!is_color_equal(label->bg, COLOR_TRANSPARENT))
        {
            for (int cy = 0; cy < element->properties.h; ++cy)
            {
                for (int cx = 0; cx < element->properties.w; ++cx)
                {
                    Renderer_Draw_Cell(r, abs_x + cx, abs_y + cy, ' ', label->fg, label->bg);
                }
            }
        }

        const char *str = label->textCallback(label->ud);
        if (str)
        {
            // Button text is always centered within its own bounding box, for a classic button look.
            draw_text(r, str, abs_x, abs_y, element->properties.w, element->properties.h, label->fg, COLOR_TRANSPARENT, ALIGN_CENTER);
        }
        break;
    }
    default:
        break;
    }
}

/**
 * @brief Draws all elements in an overlay using a simple, explicit positioning model.
 */
static void draw_ui_overlay(Renderer *r, UISurface *surface, UIOverlay *overlay)
{
    if (!r || !surface || !overlay)
        return;

    // Calculate the absolute top-left corner of the overlay.
    int overlay_abs_x = surface->properties.x + overlay->properties.x;
    int overlay_abs_y = surface->properties.y + overlay->properties.y;

    // --- REFINED: Define the container for alignment. ---
    // If the overlay has its own width, that's the container. Otherwise, it's the parent surface.
    int container_w = overlay->properties.w > 0 ? overlay->properties.w : surface->properties.w;

    // Iterate through elements and draw them.
    for (int i = 0; i < overlay->elements_count; ++i)
    {
        UIElement *element = overlay->elements[i];
        if (!element->properties.visible)
            continue;

        int element_abs_x;

        // --- REFINED: Alignment logic is now applied to the ELEMENT'S POSITION. ---
        if (overlay->align == ALIGN_CENTER)
        {
            // Center the element within the container, then apply its own x as an offset.
            element_abs_x = overlay_abs_x + (container_w - element->properties.w) / 2 + element->properties.x;
        }
        else if (overlay->align == ALIGN_RIGHT)
        {
            // Right-align the element, then apply its own x as an offset.
            element_abs_x = overlay_abs_x + container_w - element->properties.w + element->properties.x;
        }
        else // ALIGN_LEFT
        {
            // Default left-alignment.
            element_abs_x = overlay_abs_x + element->properties.x;
        }

        // The element's final Y position is its own y offset from the overlay's absolute position.
        int element_abs_y = overlay_abs_y + element->properties.y;

        // Delegate the final drawing call to the element-specific function.
        if (element->properties.visible && element->properties.enabled)
            draw_ui_element(r, overlay, element, element_abs_x, element_abs_y);
    }
}

/**
 * @brief Draws a surface and all its child overlays.
 */
static void draw_ui_surface(Renderer *r, UISurface *s)
{
    if (s->cells && s->cell_count > 0)
    {
        int width = s->properties.w;
        for (int i = 0; i < s->cell_count; i++)
        {
            Bzzt_Cell c = s->cells[i];
            if (c.visible)
            {
                int x = s->properties.x + (i % width);
                int y = s->properties.y + (i / width);
                unsigned char glyph = c.glyph == 255 ? 32 : c.glyph;
                Renderer_Draw_Cell(r, x, y, glyph, c.fg, c.bg);
            }
        }
    }

    for (int i = 0; i < s->overlays_count; ++i)
    {
        UIOverlay *o = s->overlays[i];
        if (o->properties.visible)
            draw_ui_overlay(r, s, o);
    }
}

/**
 * @brief Draws a layer and all its child surfaces.
 */
static void draw_ui_layer(Renderer *r, UILayer *l)
{
    for (int i = 0; i < l->surface_count; ++i)
    {
        UISurface *s = l->surfaces[i];
        if (s->properties.visible)
        {
            draw_ui_surface(r, s);
        }
    }
}

/**
 * @brief Entry point for drawing the entire UI tree.
 */
void Renderer_Draw_UI(Renderer *r, const UI *ui)
{
    if (!ui || !ui->visible)
        return;

    for (int i = 0; i < ui->layer_count; ++i)
    {
        UILayer *l = ui->layers[i];
        if (l->visible)
            draw_ui_layer(r, l);
    }
}