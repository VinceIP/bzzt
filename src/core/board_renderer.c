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

    // Clear the board
    clear_board(r, b);

    // Step 1: Draw all static tiles (non-stat tiles)
    for (int y = 0; y < b->height; ++y)
    {
        for (int x = 0; x < b->width; ++x)
        {
            Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, x, y);
            Bzzt_Stat *stat = Bzzt_Board_Get_Stat_At(b, x, y);

            // Only draw tiles that don't have stats on them
            // OR draw the under-tile if stat is present
            if (!stat)
            {
                if (tile.blink && w->allow_blink && !w->blink_state)
                {
                    // Blink off - draw background
                    if (tile.element == ZZT_WATER)
                        Renderer_Draw_Cell(r, x, y, ' ', tile.bg,
                                           tile.bg);
                }
                else if (tile.visible)
                {
                    Renderer_Draw_Cell(r, x, y, tile.glyph, tile.fg,
                                       tile.bg);
                }
            }
            else
            {
                // Draw under-tile for this stat's position
                if (stat->under.visible)
                    Renderer_Draw_Cell(r, x, y, stat->under.glyph,
                                       stat->under.fg, stat->under.bg);
            }
        }
    }

    for (int i = 0; i < b->stat_count; ++i)
    {
        Bzzt_Stat *stat = b->stats[i];
        if (!stat)
            continue;

        // Get interpolated position
        float render_x, render_y;
        Bzzt_Get_Interpolated_Position(w, stat, &render_x, &render_y);

        // Get the tile at the stat's LOGICAL position (not interpolated)
        Bzzt_Tile tile = Bzzt_Board_Get_Tile(b, stat->x, stat->y);

        // Check if we should draw this stat
        bool should_draw = true;

        // Handle blinking (paused player)
        if (tile.blink && w->allow_blink && !w->blink_state)
        {
            should_draw = false; // Don't draw stat during blink-off
        }

        // Don't draw player on title screen
        if (w->boards_current == 0 && tile.element == ZZT_PLAYER)
        {
            should_draw = false;
        }

        if (should_draw && tile.visible)
        {
            Renderer_Draw_Cell_Float(r, render_x, render_y,
                                     tile.glyph, tile.fg, tile.bg);
        }
    }
}
