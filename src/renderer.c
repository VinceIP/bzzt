#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "renderer.h"
#include "engine.h"

bool Renderer_Init(Renderer *r, const char *path)
{
    Image img = LoadImage(path);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    // ImageColorReplace(&img, (Color){0, 0, 0, 255}, BLANK);
    // ImageColorReplace(&img, (Color){8, 8, 8, 255}, BLANK);
    // ImageColorReplace(&img, (Color){16, 16, 16, 255}, BLANK);
    r->font = LoadTextureFromImage(img);
    UnloadImage(img);
    if (r->font.id == 0)
        return false;
    SetTextureFilter(r->font, TEXTURE_FILTER_POINT);
    r->glyph_w = 16;
    r->glyph_h = 32;

    Vector2 centerCoord = {(float)GetRenderWidth() / 2, (float)GetRenderHeight() / 2};
    r->centerCoord = centerCoord;

    r->inStr = "\n\nL - load world\nE - editor\nESC - quit";

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

static void draw_text_centered(Font f, const char *msg, Vector2 center, float size, float spacing, Color tint)
{
    Vector2 ext = MeasureTextEx(f, msg, size, spacing);
    Vector2 origin = {ext.x * 0.5f, ext.y * 0.5f};
    DrawTextPro(f, msg, center, origin, 0.0f, size, spacing, tint);
}

static void draw_splash(Renderer *r, Engine *e)
{
    Vector2 c = r->centerCoord;
    draw_text_centered(e->font, "bzzt!", (Vector2){c.x, c.y - 60}, 60, 0, RAYWHITE);
    draw_text_centered(e->font, r->inStr, c, 25, 0, RAYWHITE);
}

static void draw_editor(Renderer *r, Engine *e)
{
    Vector2 c = r->centerCoord;
   draw_text_centered(e->font, "Edit mode engaged. Press Q to go back to title.", (Vector2){c.x, c.y+20}, 30, 0, RAYWHITE);
}

static void draw_cursor(Renderer *r, Engine *e)
{
    Cursor *c = &e->cursor;
    double now = GetTime();

    if(now - c->lastBlink >= c->blinkRate){
        printf("blinking");
        c->visible = !c->visible;
        c->lastBlink = now;
    }

    if(c->visible){
        draw_cell(r, c->x, c->y, c->glyph, c->color, COLOR_BLACK);
    }
}

void Renderer_Update(Renderer *r, Engine *e)
{
    switch (e->state)
    {
    case SPLASH_MODE:
        draw_splash(r, e);
        break;

    case EDIT_MODE:
        draw_editor(r, e);
        draw_cursor(r, e);
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
