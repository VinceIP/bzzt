/**
 * @file ui_renderer.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief An efficient UI renderer with separated anchoring and alignment.
 * @version 0.3
 * @date 2025-08-10
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
 * @brief Gets metrics for the next line in a string in a single pass.
 *
 * @param str_ptr Pointer to a string pointer. The function updates this pointer to the beginning of the next line.
 * @return LineMetrics Struct containing the line's start, end, and visible width.
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
 * @brief Draws a string of text, aligning it horizontally within a bounding box.
 * @param align The horizontal alignment for each line.
 */
static void draw_text(Renderer *r, const char *str,
                      int x, int y, int maxW, int maxH,
                      Color_Bzzt fg, Color_Bzzt bg,
                      UIAlign align)
{
    if (!r || !str || !*str)
        return;

    // Sanitize bounding box dimensions to prevent issues with negative values.
    int safe_maxW = MAX(0, maxW);
    int safe_maxH = MAX(0, maxH);

    int cursor_y = y;
    const char *p = str;

    while (*p)
    {
        // Perform vertical clipping before processing the line.
        if (safe_maxH > 0 && (cursor_y < y || cursor_y >= y + safe_maxH))
        {
            get_next_line_metrics(&p); // Advance pointer past the clipped line.
            cursor_y++;
            continue;
        }

        LineMetrics line = get_next_line_metrics(&p);

        // Calculate the starting X position for this specific line based on its alignment.
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
            // Handle color escape codes.
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
                if (type == 'f')
                    current_fg = bzzt_get_color(atoi(num_buf));
                else
                    current_bg = bzzt_get_color(atoi(num_buf));
                continue;
            }

            // Draw character with horizontal clipping.
            if (safe_maxW <= 0 || (cursor_x >= x && cursor_x < x + safe_maxW))
            {
                if (cursor_x >= 0 && cursor_y >= 0) // Prevent drawing at negative screen coordinates.
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
 * @brief Measures the total size of an overlay's content based on its layout.
 */
static void measure_overlay_content(UIOverlay *overlay, int *content_w, int *content_h)
{
    if (!overlay || !content_w || !content_h)
        return;

    *content_w = 0;
    *content_h = 0;

    if (overlay->layout == LAYOUT_VBOX)
    {
        for (int i = 0; i < overlay->elements_count; ++i)
        {
            UIElement *e = overlay->elements[i];
            *content_w = MAX(*content_w, e->properties.w);
            *content_h += e->properties.h;
            if (i < overlay->elements_count - 1)
            {
                *content_h += overlay->spacing;
            }
        }
    }
    // else if (overlay->layout == LAYOUT_HBOX) { /* Future hbox implementation would go here. */ }
    else // LAYOUT_NONE
    {
        // For layout none, size is the union of all element bounding boxes.
        for (int i = 0; i < overlay->elements_count; ++i)
        {
            UIElement *e = overlay->elements[i];
            *content_w = MAX(*content_w, e->properties.x + e->properties.w);
            *content_h = MAX(*content_h, e->properties.y + e->properties.h);
        }
    }
}

/**
 * @brief Draws all elements in an overlay, handling anchoring and alignment.
 */
static void draw_ui_overlay(Renderer *r, UISurface *surface, UIOverlay *overlay)
{
    if (!r || !surface || !overlay)
        return;

    // 1. Measure the total size of the overlay's content.
    int content_w, content_h;
    measure_overlay_content(overlay, &content_w, &content_h);

    // If overlay has explicit dimensions in the BUI file, use them. Otherwise, use the measured content size.
    int overlay_w = overlay->properties.w > 0 ? overlay->properties.w : content_w;
    int overlay_h = overlay->properties.h > 0 ? overlay->properties.h : content_h;

    // 2. Calculate the overlay's top-left (ox, oy) based on its anchor to the parent surface.
    int ox = surface->properties.x + overlay->properties.x;
    int oy = surface->properties.y + overlay->properties.y;

    // Horizontal Anchoring
    if (overlay->anchor == ANCHOR_TOP_CENTER || overlay->anchor == ANCHOR_MIDDLE_CENTER || overlay->anchor == ANCHOR_BOTTOM_CENTER)
        ox = surface->properties.x + (surface->properties.w - overlay_w) / 2;
    else if (overlay->anchor == ANCHOR_TOP_RIGHT || overlay->anchor == ANCHOR_MIDDLE_RIGHT || overlay->anchor == ANCHOR_BOTTOM_RIGHT)
        ox = surface->properties.x + surface->properties.w - overlay_w;

    // Vertical Anchoring
    if (overlay->anchor == ANCHOR_MIDDLE_LEFT || overlay->anchor == ANCHOR_MIDDLE_CENTER || overlay->anchor == ANCHOR_MIDDLE_RIGHT)
        oy = surface->properties.y + (surface->properties.h - overlay_h) / 2;
    else if (overlay->anchor == ANCHOR_BOTTOM_LEFT || overlay->anchor == ANCHOR_BOTTOM_CENTER || overlay->anchor == ANCHOR_BOTTOM_RIGHT)
        oy = surface->properties.y + surface->properties.h - overlay_h;

    // 3. Draw each element, positioning it relative to the overlay's calculated (ox, oy) and alignment.
    int cursor_y = 0; // Relative y-position for layout modes
    for (int i = 0; i < overlay->elements_count; ++i)
    {
        UIElement *element = overlay->elements[i];
        if (!element->properties.visible)
            continue;

        int element_abs_x, element_abs_y;

        // Calculate final X position based on overlay's alignment property.
        if (overlay->align == ALIGN_CENTER)
            element_abs_x = ox + (overlay_w - element->properties.w) / 2 + element->properties.x;
        else if (overlay->align == ALIGN_RIGHT)
            element_abs_x = ox + overlay_w - element->properties.w + element->properties.x;
        else // ALIGN_LEFT
            element_abs_x = ox + element->properties.x;

        // Calculate final Y position based on overlay's layout property.
        if (overlay->layout == LAYOUT_VBOX)
            element_abs_y = oy + cursor_y + element->properties.y;
        else // LAYOUT_NONE
            element_abs_y = oy + element->properties.y;

        // --- Draw the element based on its type ---
        switch (element->type)
        {
        case UI_ELEM_TEXT:
        {
            UIElement_Text *text_elem = (UIElement_Text *)element;
            const char *str = text_elem->textCallback(text_elem->ud);
            if (str)
            {
                // Text elements are drawn simply at their calculated position.
                // The alignment of the text's content is implicitly left, as it fills its own box.
                draw_text(r, str, element_abs_x, element_abs_y, element->properties.w, element->properties.h, text_elem->fg, text_elem->bg, ALIGN_LEFT);
            }
            break;
        }
        case UI_ELEM_BUTTON:
        {
            UIButton *button = (UIButton *)element;
            UIElement_Text *label = button->label;
            if (!label)
                break;

            // Draw button background at its final calculated position.
            for (int cy = 0; cy < element->properties.h; ++cy)
            {
                for (int cx = 0; cx < element->properties.w; ++cx)
                {
                    Renderer_Draw_Cell(r, element_abs_x + cx, element_abs_y + cy, ' ', label->fg, label->bg);
                }
            }

            // Draw the label text, which is always centered inside the button's own bounds.
            const char *str = label->textCallback(label->ud);
            if (str)
            {
                Color_Bzzt transparent_bg = bzzt_get_color(BZ_TRANSPARENT);
                draw_text(r, str, element_abs_x, element_abs_y, element->properties.w, element->properties.h, label->fg, transparent_bg, ALIGN_CENTER);
            }
            break;
        }
        default:
            break;
        }

        // Advance layout cursor if a layout mode is active.
        if (overlay->layout == LAYOUT_VBOX)
        {
            cursor_y += element->properties.h + overlay->spacing;
        }
    }
}

/**
 * @brief Draws a surface and all its child overlays.
 */
static void draw_ui_surface(Renderer *r, UISurface *s)
{
    // Draw the surface's base cells (background).
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

    // Draw each overlay this surface holds.
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