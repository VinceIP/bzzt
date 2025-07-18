#include "ui_renderer.h"
#include "ui.h"
#include "text.h"
#include "code_page_lut.h"
#include "object.h"
#include <string.h>
#include <stdio.h>

static void draw_ui_element(Renderer *r, UISurface *s, UIOverlay *ov, UIElement *e)
{
    int base_x = s->x + ov->x + e->x;
    int base_y = s->y + ov->y + e->y;

    switch (e->type)
    {
    case UI_ELEM_TEXT:
    {
        UIElement_Text *t = (UIElement_Text *)e;
        const char *str = t->textCallback(t->ud);
        int x = base_x;
        int y = base_y;
        int maxWidth = t->wrap ? s->w - (x - s->x) : s->w;
        int len = strlen(str);
        for (int i = 0; i < len; ++i)
        {
            unsigned char c = (unsigned char)str[i];
            if (c == '\n')
            {
                x = base_x;
                y += 1;
                continue;
            }
            if (x - base_x >= maxWidth)
            {
                if (t->wrap)
                {
                    x = base_x;
                    y += 1;
                }
                else
                {
                    break;
                }
            }

            Renderer_Draw_Cell(r, x, y, unicode_to_cp437(c), t->fg, t->bg);
            x += 1;
        }
        break;
    }
    case UI_ELEM_BUTTON:
    {
        UIButton *b = (UIButton *)e;
        const char *caption = b->label->textCallback(b->label->ud);
        Renderer_Draw_Cell(r, base_x, base_y, '[', b->label->fg, b->label->bg);
        int x = base_x + 1;
        int len = strlen(caption);
        for (int i = 0; i < len; ++i)
        {
            Renderer_Draw_Cell(r, x + i, base_y, unicode_to_cp437((unsigned char)caption[i]), b->label->fg, b->label->bg);
        }
        Renderer_Draw_Cell(r, base_x + len + 1, base_y, ']', b->label->fg, b->label->bg);
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
        if (e->visible)
            draw_ui_element(r, s, o, e);
    }
}

static void draw_ui_surface(Renderer *r, UISurface *s)
{
    int cell_count = s->cell_count;
    Cell *cells = s->cells;
    int width = s->w;
    int height = s->h;
    // If any cells live on this surface, draw them
    if (cells && cell_count > 0)
    {
        for (int i = 0; i < cell_count; i++)
        {
            Cell c = cells[i];
            if (c.visible) // Skip invisible cells
            {
                int x = s->x + (i % width);
                int y = s->y + (i / width);
                unsigned char glyph = c.glyph == 255 ? 32 : c.glyph;
                Renderer_Draw_Cell(r, x, y, glyph, c.fg, c.bg);
            }
        }
    }

    for (int i = 0; i < s->overlays_count; ++i)
    {
        UIOverlay *o = s->overlays[i];
        if (o->visible)
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
        if (s->visible) // Skip invisible surfaces
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
