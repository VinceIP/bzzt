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

void Renderer_Draw_Board(Renderer *r, const Bzzt_Board *b)
{
    if (!r || !b || !b->objects || b->object_count == 0)
        return;

    clear_board(r, b);

    Bzzt_Object *player;

    for (int i = 0; i < b->object_count; ++i)
    {
        if (b->objects[i])
        {
            Bzzt_Object *obj = b->objects[i];
            if (obj->bzzt_type == ZZT_PLAYER)
                player = obj;
            if (obj != player)
                Renderer_Draw_Cell(r, obj->x, obj->y, obj->cell.glyph,
                                   obj->cell.fg, obj->cell.bg);
        }
    }

    Renderer_Draw_Cell(r, player->x, player->y, player->cell.glyph, player->cell.fg, player->cell.bg);
}