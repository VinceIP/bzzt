#include <stdbool.h>
#include <stdlib.h>
#include "raylib.h"
#include "renderer.h"
#include "engine.h"

bool Renderer_Init(Renderer *r, const char *path)
{
    Image img = LoadImage(path);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageColorReplace(&img, (Color){0, 0, 0, 255}, BLANK);    // pure black
    ImageColorReplace(&img, (Color){8, 8, 8, 255}, BLANK);    // dark grey 1
    ImageColorReplace(&img, (Color){16, 16, 16, 255}, BLANK); // dark grey 2    r->font = LoadTextureFromImage(img);
    r->font = LoadTextureFromImage(img);
    UnloadImage(img);
    if (r->font.id == 0)
        return false;
    SetTextureFilter(r->font, TEXTURE_FILTER_POINT);
    r->glyph_w = 16;
    r->glyph_h = 32;
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

void Renderer_Update(Renderer *r, Engine *e){
    switch (e->state)
    {
    case SPLASH_MODE:
        draw_splash(r);
        break;
    
    default:
        break;
    }
}

void Renderer_DrawBoard(Renderer *r, Board *b)
{
    static const Object empty = {.glyph = '0', .fg_color = COLOR_BLACK, .bg_color = COLOR_BLACK};
    const Object *grid[b->height][b->width];
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

void Renderer_Quit(Renderer *r)
{
    UnloadTexture(r->font);
}

static void draw_splash(Renderer *r)
{
    int cx = GetRenderWidth()/2;
    int cy = GetRenderHeight()/2;
    DrawText("bzzt!",cx,cy,30, RAYWHITE);
    DrawText("L - load world", cx, cy+30, 20, RAYWHITE);
    DrawText("E - editor", cx, cy + 50, 20, RAYWHITE);
}