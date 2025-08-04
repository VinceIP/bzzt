#include "ui_renderer.h"
#include "ui.h"
#include "code_page_lut.h"
#include "bzzt.h"
#include <string.h>
#include <ctype.h>

static void measure_text(const char *str, int *w, int *h)
{
    int max_w = 0, cur_w = 0, lines = 1;
    if (!str)
    {
        if (w)
            *w = 0;
        if (h)
            *h = 0;
        return;
    }
    for (int i = 0; str[i];)
    {
        if (str[i] == '\\')
        {
            if (str[i + 1] == 'n')
            {
                if (cur_w > max_w)
                    max_w = cur_w;
                cur_w = 0;
                lines++;
                i += 2;
                continue;
            }
            else if ((str[i + 1] == 'f' && str[i + 2] == 'g') || (str[i + 1] == 'b' && str[i + 2] == 'g'))
            {
                i += 3;
                while (str[i] && isdigit((unsigned char)str[i]))
                    i++;
                continue;
            }
        }
        cur_w++;
        i++;
    }
    if (cur_w > max_w)
        max_w = cur_w;
    if (w)
        *w = max_w;
    if (h)
        *h = lines;
}

static void draw_escaped_text(Renderer *r, const char *str, int base_x, int base_y, int max_w, int max_h, bool wrap, Color_Bzzt fg, Color_Bzzt bg)
{
    int x = base_x;
    int y = base_y;
    Color_Bzzt cur_fg = fg;
    Color_Bzzt cur_bg = bg;
    for (int i = 0; str[i];)
    {
        if (str[i] == '\\')
        {
            if (str[i + 1] == 'n')
            {
                x = base_x;
                y += 1;
                i += 2;
                if (y - base_y >= max_h)
                    break;
                continue;
            }
            else if (str[i + 1] == 'f' && str[i + 2] == 'g')
            {
                i += 3;
                int val = 0;
                while (str[i] && isdigit((unsigned char)str[i]))
                {
                    val = val * 10 + (str[i] - '0');
                    i++;
                }
                cur_fg = bzzt_get_color(val);
                continue;
            }
            else if (str[i + 1] == 'b' && str[i + 2] == 'g')
            {
                i += 3;
                int val = 0;
                while (str[i] && isdigit((unsigned char)str[i]))
                {
                    val = val * 10 + (str[i] - '0');
                    i++;
                }
                cur_bg = bzzt_get_color(val);
                continue;
            }
        }

        if (x - base_x >= max_w)
        {
            if (wrap)
            {
                x = base_x;
                y += 1;
                if (y - base_y >= max_h)
                    break;
            }
            else
            {
                break;
            }
        }
        if (y - base_y >= max_h)
            break;
        Renderer_Draw_Cell(r, x, y, unicode_to_cp437((unsigned char)str[i]), cur_fg, cur_bg);
        x += 1;
        i++;
    }
}

static void draw_ui_element(Renderer *r, UISurface *s, UIOverlay *ov, UIElement *e)
{
    int base_x = s->properties.x + ov->properties.x + e->properties.x;
    int base_y = s->properties.y + ov->properties.y + e->properties.y;

    // Handle anchoring
    int elem_width = 0, elem_height = 1;
    if (e->type == UI_ELEM_TEXT)
    {
        UIElement_Text *t = (UIElement_Text *)e;
        const char *str = t->textCallback(t->ud);
        measure_text(str, &elem_width, &elem_height);
        if (e->properties.expand)
        {
            int max_w = ov->properties.w - e->properties.x;
            int max_h = ov->properties.h - e->properties.y;
            if (elem_width > e->properties.w)
                e->properties.w = elem_width > max_w ? max_w : elem_width;
            if (elem_height > e->properties.h)
                e->properties.h = elem_height > max_h ? max_h : elem_height;
        }
    }
    else if (e->type == UI_ELEM_BUTTON)
    {
        UIButton *b = (UIButton *)e;
        const char *caption = b->label->textCallback(b->label->ud);
        measure_text(caption, &elem_width, &elem_height);
        if (e->properties.expand)
        {
            int max_w = ov->properties.w - e->properties.x;
            int max_h = ov->properties.h - e->properties.y;
            if (elem_width > e->properties.w)
                e->properties.w = elem_width > max_w ? max_w : elem_width;
            if (elem_height > e->properties.h)
                e->properties.h = elem_height > max_h ? max_h : elem_height;
        }
    }

    int anchor_offset = 0;
    if (ov->anchor == ANCHOR_CENTER)
        anchor_offset = (ov->properties.w - elem_width) / 2;
    else if (ov->anchor == ANCHOR_RIGHT)
        anchor_offset = ov->properties.w - elem_width;
    base_x += anchor_offset;
    //

    switch (e->type)
    {
    case UI_ELEM_TEXT:
    {
        UIElement_Text *t = (UIElement_Text *)e;
        const char *str = t->textCallback(t->ud);
        int max_w = e->properties.w > 0 ? e->properties.w : ov->properties.w - e->properties.x;
        int max_h = e->properties.h > 0 ? e->properties.h : ov->properties.h - e->properties.y;
        draw_escaped_text(r, str, base_x, base_y, max_w, max_h, t->wrap, t->fg, t->bg);
        break;
    }
    case UI_ELEM_BUTTON:
    {
        UIButton *b = (UIButton *)e;
        const char *caption = b->label->textCallback(b->label->ud);
        int max_w = e->properties.w > 0 ? e->properties.w : ov->properties.w - e->properties.x;
        int max_h = e->properties.h > 0 ? e->properties.h : ov->properties.h - e->properties.y;
        draw_escaped_text(r, caption, base_x, base_y, max_w, max_h, false, b->label->fg, b->label->bg);
        break;
    }
    default:
        break;
    }
}

static void draw_ui_overlay(Renderer *r, UISurface *s, UIOverlay *o)
{
    for (int i = 0; i < o->elements_count; ++i)
    {
        UIElement *e = o->elements[i];
        if (e->properties.visible)
            draw_ui_element(r, s, o, e);
    }
}

static void draw_ui_surface(Renderer *r, UISurface *s)
{
    int cell_count = s->cell_count;
    Bzzt_Cell *cells = s->cells;
    int width = s->properties.w;
    int height = s->properties.h;
    // If any cells live on this surface, draw them
    if (cells && cell_count > 0)
    {
        for (int i = 0; i < cell_count; i++)
        {
            Bzzt_Cell c = cells[i];
            if (c.visible) // Skip invisible cells
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

static void draw_ui_layer(Renderer *r, UILayer *l)
{
    // Draw any surfaces on this layer
    int surface_count = l->surface_count;
    for (int i = 0; i < surface_count; ++i)
    {
        UISurface *s = l->surfaces[i];
        if (s->properties.visible) // Skip invisible surfaces
        {
            draw_ui_surface(r, s);
        }
    }
}

void Renderer_Draw_UI(Renderer *r, const UI *ui)
{

    if (!ui || !ui->visible)
    {
        return; // Skip drawing if UI is NULL or invisible
    }

    // Draw any existing UI layers
    int layer_count = ui->layer_count;
    for (int i = 0; i < layer_count; ++i)
    {
        UILayer *l = ui->layers[i];
        if (l->visible) // Skip invisible layers
            draw_ui_layer(r, l);
    }
}
