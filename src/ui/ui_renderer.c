#include "ui_renderer.h"
#include "ui.h"
#include "ui_layer.h"
#include "ui_surface.h"
#include "ui_element.h"
#include "object.h"

static void draw_ui_element(Renderer *r, UIElement *e)
{
    (void)r;
    (void)e;
}

static void draw_ui_surface(Renderer *r, UISurface *s)
{
    int cell_count = s->cell_count;
    Cell *cells = s->cells;
    int width = s->w;
    // If any cells live on this surface, draw them
    if (cells && cell_count > 0)
    {
        for (int i = 0; i < cell_count; i++)
        {
            Cell c = cells[i];
            if (c.visible) // Skip invisible cells
            {
                int x = i % width;
                int y = i / width;
                if (c.glyph != 255) // Skip "transparent" glyph
                    Renderer_Draw_Cell(r, x, y, c.glyph, c.fg, c.bg);
            }
        }
    }

    // If any sub elements live on this surface, draw them over the cells
    int elements_count = s->elements_count;
    if (elements_count > 0)
    {
        for (int i = 0; i < elements_count; ++i)
        {
            UIElement *e = s->elements[i];
            if (e->visible) // Skip invisible elements
                draw_ui_element(r, e);
        }
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
            draw_ui_surface(r, s);
    }
}

void Renderer_Draw_UI(Renderer *r, const UI *ui)
{
    if (!ui || !ui->visible)
        return; // Skip drawing if UI is NULL or invisible

    // Draw any existing UI layers
    int layer_count = ui->layer_count;
    for (int i = 0; i < layer_count; ++i)
    {
        UILayer *l = ui->layers[i];
        if (l->visible) // Skip invisible layers
            draw_ui_layer(r, l);
    }
}