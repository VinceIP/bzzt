#include "board_renderer.h"
#include "renderer.h"
#include "bzzt.h"

// static const Bzzt_Object empty = {
//     .id = 0,
//     .x = 0,
//     .y = 0,
//     .dir = DIR_NONE,
//     .cell = {
//         .visible = false,
//         .glyph = 0,
//         .fg = COLOR_BLACK,
//         .bg = COLOR_BLACK,
//     },
// };

// const Bzzt_Object *grid[b->height][b->width];

// void Renderer_Draw_Board(Renderer *r, const Bzzt_Board *b)
// {

//     // Initialize board as empty objects
//     for (int y = 0; y < b->height; ++y)
//     {
//         for (int x = 0; x < b->width; ++x)
//         {
//             grid[y][x] = &empty;
//         }
//     }

//     // Overlay live objects on this board
//     for (int i = 0; i < b->object_count; ++i)
//     {
//         Bzzt_Object *o = b->objects[i];
//         grid[o->y][o->x] = o;
//     }

//     for (int y = 0; y < b->height; ++y)
//     {
//         for (int x = 0; x < b->width; ++x)
//         {
//             const Bzzt_Object *o = grid[y][x];
//             Renderer_Draw_Cell(r, x, y, o->cell.glyph, o->cell.fg, o->cell.bg);
//         }
//     }
// }

static void clear_board(Renderer *r, Bzzt_Board *b)
{
    int count = b->width * b->height;

    for (size_t i = 0; i < count; ++i)
    {
        int col = (int)(i % b->width);
        int row = (int)(i / b->width);
        Renderer_Draw_Cell(r, col, row, 0, COLOR_BLACK, COLOR_BLACK);
    }
}

void Renderer_Draw_Board(Renderer *r, Bzzt_World *w, const Bzzt_Board *b)
{
    if (!r || !b)
        return;

    // tbd - update only changed tiles rather than clear every frame
    clear_board(r, b);

    // draw tiles
    for (int y = 0; y < b->height; ++y)
    {
        for (int x = 0; x < b->width; ++x)
        {
            Bzzt_Stat *stat = Bzzt_Board_Get_Stat_At(w->boards[w->boards_current], x, y);
            Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
            if (tile.blink && w->allow_blink && !w->blink_state)
            {
                // blink off
                if (tile.element == ZZT_WATER)
                    Renderer_Draw_Cell(r, x, y, ' ', tile.bg, tile.bg);
                // For blink enabled stats, draw under tile if any (mainly for blinking player when paused)
                else
                {
                    if (stat)
                        Renderer_Draw_Cell(r, x, y, stat->under.glyph, stat->under.fg, stat->under.bg);
                }
            }
            // Quick hack - skip drawing player if on title screen, but draw its under tile if any
            else if (w->boards_current == 0 && tile.element == ZZT_PLAYER)
                Renderer_Draw_Cell(r, x, y, stat->under.glyph, stat->under.fg, stat->under.bg);
            // Otherwise, draw tile normally
            else
            {
                Renderer_Draw_Cell(r, x, y, tile.glyph, tile.fg, tile.bg);
            }
        }
    }
}