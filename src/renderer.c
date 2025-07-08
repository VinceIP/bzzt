#include <stdbool.h>
#include <stdlib.h>
#include "raylib.h"
#include "renderer.h"

bool Renderer_Init(Renderer *r, const char *path)
{
    Image img = LoadImage(path);
    //ImageColorReplace(&img, (Color){0,0,0,255}, (Color){0,0,0,0});
    r->font = LoadTextureFromImage(img);
    UnloadImage(img);
    if (r->font.id == 0)
        return false;
    SetTextureFilter(r->font, TEXTURE_FILTER_POINT);
    r->glyph_w = 16;
    r->glyph_h = 24;
    return true;
}

static Rectangle glyph_rec(Renderer *r, unsigned char ascii)
{
    int col = ascii % 32;
    int row = ascii / 32;
    return (Rectangle){
        col * r->glyph_w,
        row * r->glyph_h,
        r->glyph_w,
        r->glyph_h};
}

static void draw_cell(Renderer *r, int cellX, int cellY, unsigned char glyph, Color_bzzt fg, Color_bzzt bg)
{
    // Convert to RayLib color
    Color rf = (Color){fg.r, fg.g, fg.b, 255};
    Color rb = (Color){bg.r, bg.g, bg.b, 255};

    Rectangle dst = {
        cellX * r->glyph_w,
        cellY * r->glyph_h,
        r->glyph_w,
        r->glyph_h};

    // Fill bg
    DrawRectangle(dst.x, dst.y, dst.width, dst.height, rb);

    // Draw fg
    DrawTextureRec(r->font, glyph_rec(r, glyph), (Vector2){dst.x, dst.y}, rf);
}

void Renderer_DrawBoard(Renderer *r, Board *b)
{
    static const Object empty = {.glyph = ' ', .fg_color = COLOR_BLACK, .bg_color = COLOR_BLACK};
    const Object *grid[b->width][b->height];

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
        Object *o = b->objects[i];
        grid[o->y][o->x] = o;
    }

    for (int y = 0; y < b->height; ++y)
    {
        for (int x = 0; x < b->width; ++x)
        {
            const Object *o = grid[y][x];
            draw_cell(r, x, y, o->glyph, o->fg_color, o->bg_color);
        }
    }
}

void Renderer_Quit(Renderer* r){
    UnloadTexture(r->font);
}