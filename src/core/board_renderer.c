#include "board_renderer.h"
#include "renderer.h"
#include "bzzt.h"

void Renderer_Draw_Board(Renderer *r, const Bzzt_Board *b)
{
    static const Bzzt_Object empty = {
        .id = 0,
        .x = 0,
        .y = 0,
        .dir = DIR_NONE,
        .cell = {
            .visible = false,
            .glyph = 0,
            .fg = COLOR_BLACK,
            .bg = COLOR_BLACK,
        },
    };
    const Bzzt_Object *grid[b->height][b->width];
    // Initialize board as empty objects
    for (int y = 0; y < b->height; ++y)
    {
        for (int x = 0; x < b->width; ++x)
        {
            grid[y][x] = &empty;
        }
    }

    // Overlay live objects on this board
    for (int i = 0; i < b->object_count; ++i)
    {
        Bzzt_Object *o = b->objects[i];
        grid[o->y][o->x] = o;
    }

    for (int y = 0; y < b->height; ++y)
    {
        for (int x = 0; x < b->width; ++x)
        {
            const Bzzt_Object *o = grid[y][x];
            Renderer_Draw_Cell(r, x, y, o->cell.glyph, o->cell.fg, o->cell.bg);
        }
    }
}