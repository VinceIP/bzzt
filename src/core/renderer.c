#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "raylib.h"
#include "renderer.h"
#include "engine.h"
#include "color.h"
#include "bzzt.h"
#include "board_renderer.h"
#include "ui_renderer.h"
#include "bz_char.h"
#include "debugger.h"

#define TRANSPARENT_GLYPH 255

static BzztCharset defaultCharset;

static Texture2D texture_from_charset(BzztCharset *c)
{
    const int cols = 32;
    const int rows = (c->header.glyph_count + cols - 1) / cols;
    const int width = c->header.glyph_w * cols;
    const int height = c->header.glyph_h * rows;

    unsigned char *data = calloc(width * height, sizeof(unsigned char));
    if (!data)
        return (Texture2D){0};

    const size_t bits_per_row = c->header.glyph_w * c->header.bpp;
    const size_t bytes_per_row = (bits_per_row + 7) / 8;
    const size_t glyph_bytes = bytes_per_row * c->header.glyph_h;

    for (int g = 0; g < c->header.glyph_count; ++g)
    {
        int gx = g % cols;
        int gy = g / cols;
        unsigned char *glyph = c->pixels + glyph_bytes * g;

        for (int y = 0; y < c->header.glyph_h; ++y)
        {
            for (int x = 0; x < c->header.glyph_w; ++x)
            {
                size_t byte_index = y * bytes_per_row + (x / 8);
                int bit_index = 7 - (x % 8);
                unsigned char bit = (glyph[byte_index] >> bit_index) & 1u;

                int tx = gx * c->header.glyph_w + x;
                int ty = gy * c->header.glyph_h + y;
                data[ty * width + tx] = bit ? 255 : 0;
            }
        }
    }

    Image img = {
        .data = data,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE};

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img); // frees data
    return tex;
}

bool Renderer_Init(Renderer *r, Engine *e, const char *path)
{
    Debug_Printf(LOG_ENGINE, "Initializing renderer.");

    if (!BZC_Load(path, &defaultCharset))
    {
        Debug_Printf(LOG_ENGINE, "Error loading charset.");
        return false;
    }

    // Build texture from charset
    r->font = texture_from_charset(&defaultCharset);
    SetTextureFilter(r->font, TEXTURE_FILTER_POINT);

    r->glyphShader = LoadShader("assets/shaders/glyph.vs", "assets/shaders/glyph.fs");
    r->glyphShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(r->glyphShader, "mvp");
    r->glyphShader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(r->glyphShader, "texture0");
    SetShaderValueTexture(r->glyphShader, r->glyphShader.locs[SHADER_LOC_MAP_DIFFUSE], r->font);

    SetTargetFPS(60);

    r->src_w = defaultCharset.header.glyph_w;
    r->src_h = defaultCharset.header.glyph_h;

    float desired = 2.0f;
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int maxScaleW = screenW / (BZZT_BOARD_DEFAULT_W * r->src_w);
    int maxeScaleH = screenH / (BZZT_BOARD_DEFAULT_H * r->src_h);
    int maxScale = maxScaleW < maxeScaleH ? maxScaleW : maxeScaleH;
    if (desired > maxScale)
        desired = maxScale;
    if (desired < 1)
        desired = 1;
    r->scale = (float)desired;
    r->glyph_w = r->src_w * desired;
    r->glyph_h = r->src_h * desired;

    Vector2 centerCoord = {(float)GetRenderWidth() / 2, (float)GetRenderHeight() / 2};
    r->centerCoord = centerCoord;

    return true;
}

static Rectangle glyph_rec(Renderer *r, unsigned char ascii)
{
    int col = ascii % 32;
    int row = ascii / 32;
    return (Rectangle){
        col * r->src_w,
        row * r->src_h,
        r->src_w,
        r->src_h};
}

void Renderer_Draw_Cell(Renderer *r, int cellX, int cellY, unsigned char glyph, Color_Bzzt fg, Color_Bzzt bg)
{
    // Convert to RayLib color
    Color rf = (Color){fg.r, fg.g, fg.b, 255};
    Color rb = (Color){bg.r, bg.g, bg.b, 255};

    Rectangle src = glyph_rec(r, glyph);
    Rectangle dst = {
        cellX * r->glyph_w,
        cellY * r->glyph_h,
        r->glyph_w,
        r->glyph_h};

    if (!(bg.r == COLOR_TRANSPARENT.r && bg.g == COLOR_TRANSPARENT.g && bg.b == COLOR_TRANSPARENT.b))
    {
        DrawRectangleRec(dst, rb);
    }
    DrawTexturePro(r->font, src, dst, (Vector2){0, 0}, 0.0f, rf);
}

static void draw_cursor(Renderer *r, Engine *e)
{
    if (!r || !e || !e->cursor)
        return;
    double now = GetTime();

    if (now - e->cursor->lastBlink >= e->cursor->blinkRate)
    {
        e->cursor->visible = !e->cursor->visible;
        e->cursor->lastBlink = now;
    }

    if (e->cursor->visible)
    {
        Renderer_Draw_Cell(r, e->cursor->position.x, e->cursor->position.y, e->cursor->glyph, e->cursor->color, COLOR_BLACK);
    }
}

void Renderer_Update(Renderer *r, Engine *e)
{
    ClearBackground(BLACK);
    BeginShaderMode(r->glyphShader);
    switch (e->state)
    {
    case ENGINE_STATE_SPLASH:
        //Renderer_Draw_Board(r, e->world->boards[e->world->boards_current]);
        if (e->ui)
        {
            Renderer_Draw_UI(r, e->ui);
        }
        break;
    case ENGINE_STATE_PLAY:
        Renderer_Draw_Board(r, e->world->boards[e->world->boards_current]);
        break;

    case ENGINE_STATE_EDIT:
        UI *ui = e->ui;
        if (ui)
        {
            Renderer_Draw_UI(r, ui);
        }
        draw_cursor(r, e);
        break;

    default:
        break;
    }
    EndShaderMode();
}

void Renderer_Quit(Renderer *r)
{
    UnloadTexture(r->font);
    UnloadShader(r->glyphShader);
    if (defaultCharset.pixels)
    {
        free(defaultCharset.pixels);
        defaultCharset.pixels = NULL;
    }
}
