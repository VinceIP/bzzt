#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
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
static int renderer_logical_w = 0;
static int renderer_logical_h = 0;

static Rectangle output_rect_for_size(int logical_w, int logical_h, int available_w, int available_h)
{
    if (logical_w <= 0 || logical_h <= 0 || available_w <= 0 || available_h <= 0)
        return (Rectangle){0};

    int scale_w = available_w / logical_w;
    int scale_h = available_h / logical_h;
    int integer_scale = scale_w < scale_h ? scale_w : scale_h;

    if (integer_scale >= 1)
    {
        int width = logical_w * integer_scale;
        int height = logical_h * integer_scale;

        return (Rectangle){
            (float)((available_w - width) / 2),
            (float)((available_h - height) / 2),
            (float)width,
            (float)height};
    }

    float scale_x = (float)available_w / (float)logical_w;
    float scale_y = (float)available_h / (float)logical_h;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    int width = (int)floorf((float)logical_w * scale);
    int height = (int)floorf((float)logical_h * scale);

    return (Rectangle){
        (float)((available_w - width) / 2),
        (float)((available_h - height) / 2),
        (float)width,
        (float)height};
}

static Rectangle render_output_rect(int logical_w, int logical_h)
{
    return output_rect_for_size(logical_w, logical_h, GetRenderWidth(), GetRenderHeight());
}

static Rectangle window_output_rect(int logical_w, int logical_h)
{
    return output_rect_for_size(logical_w, logical_h, GetScreenWidth(), GetScreenHeight());
}

static void refresh_present_rect(Renderer *r)
{
    if (!r)
        return;

    r->present_rect = render_output_rect(r->logical_w, r->logical_h);
}

static Texture2D texture_from_charset(BzztCharset *c)
{
    if (!c)
    {
        Debug_Printf(LOG_ENGINE, "texture_from_charset: NULL charset pointer");
        return (Texture2D){0};
    }

    if (c->header.glyph_w <= 0 || c->header.glyph_h <= 0 || c->header.glyph_count <= 0)
    {
        Debug_Printf(LOG_ENGINE, "texture_from_charset: bad header (w:%d h:%d count:%d)",
                     c->header.glyph_w, c->header.glyph_h, c->header.glyph_count);
        return (Texture2D){0};
    }

    if (!c->pixels)
    {
        Debug_Printf(LOG_ENGINE, "texture_from_charset: pixels is NULL (loader didn't populate)");
        return (Texture2D){0};
    }

    const int cols = 32; // atlas columns (must match glyph_rec)
    const int rows = (c->header.glyph_count + cols - 1) / cols;
    const int width = c->header.glyph_w * cols;
    const int height = c->header.glyph_h * rows;
    Debug_Printf(LOG_ENGINE, "stats:\nrows= %d\nw= %d\nh= %d", rows, width, height);

    const size_t total_pixels = (size_t)width * (size_t)height;

    // bytes per source glyph row (supports bpp != 1, though we only implement 1 and 8 below)
    const size_t bits_per_row = (size_t)c->header.glyph_w * (size_t)c->header.bpp;
    const size_t bytes_per_row = (bits_per_row + 7u) / 8u;
    const size_t glyph_bytes = bytes_per_row * (size_t)c->header.glyph_h;

    // Build an RGBA atlas; each pixel takes 4 bytes
    const size_t atlas_bytes = total_pixels * 4u;
    Debug_Printf(LOG_ENGINE, "atlas bytes: %zu", atlas_bytes);

    unsigned char *data = (unsigned char *)calloc(atlas_bytes, sizeof(unsigned char));
    if (!data)
    {
        Debug_Printf(LOG_ENGINE, "texture_from_charset: out of memory for atlas (%zu bytes)", atlas_bytes);
        return (Texture2D){0};
    }

    // Build a grayscale atlas image from packed glyphs, expanding to RGBA
    for (int g = 0; g < c->header.glyph_count; ++g)
    {
        const int gx = g % cols;
        const int gy = g / cols;
        const size_t g_off = glyph_bytes * (size_t)g;
        const unsigned char *glyph = c->pixels + g_off;

        for (int y = 0; y < c->header.glyph_h; ++y)
        {
            for (int x = 0; x < c->header.glyph_w; ++x)
            {
                unsigned char value = 0; // grayscale 0..255

                if (c->header.bpp == 1)
                {
                    const size_t byte_index = (size_t)y * bytes_per_row + (size_t)(x >> 3);
                    const int bit_index = 7 - (x & 7);
                    if (byte_index < glyph_bytes)
                    {
                        const unsigned char b = glyph[byte_index];
                        value = (unsigned char)(((b >> bit_index) & 1u) ? 255u : 0u);
                    }
                }
                else if (c->header.bpp == 8)
                {
                    const size_t byte_index = (size_t)y * bytes_per_row + (size_t)x;
                    if (byte_index < glyph_bytes)
                    {
                        value = glyph[byte_index];
                    }
                }
                else
                {
                    // Unsupported bpp right now; clean up and fail safely to avoid UB
                    Debug_Printf(LOG_ENGINE, "texture_from_charset: unsupported bpp=%d", c->header.bpp);
                    free(data);
                    return (Texture2D){0};
                }

                const int tx = gx * c->header.glyph_w + x;
                const int ty = gy * c->header.glyph_h + y;
                const size_t index = ((size_t)ty * (size_t)width + (size_t)tx) * 4u;
                if (index + 3 < atlas_bytes)
                {
                    data[index + 0] = value;
                    data[index + 1] = value;
                    data[index + 2] = value;
                    data[index + 3] = 255u;
                }
            }
        }
    }

    Image img = {
        .data = data,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img); // frees data

    if (!IsTextureValid(tex))
    {
        Debug_Printf(LOG_ENGINE, "texture_from_charset: LoadTextureFromImage failed");
        return (Texture2D){0};
    }

    Debug_Printf(LOG_ENGINE, "texture_from_charset: done");
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

    if (!defaultCharset.pixels)
    {
        Debug_Printf(LOG_ENGINE, "Renderer_Init: loaded charset has NULL pixels");
        return false;
    }

    // Build texture from charset
    r->font = texture_from_charset(&defaultCharset);
    if (!IsTextureValid(r->font))
    {
        Debug_Printf(LOG_ENGINE, "Failed to build texture.");
        // Avoid leaking charset if we abort here
        if (defaultCharset.pixels)
        {
            free(defaultCharset.pixels);
            defaultCharset.pixels = NULL;
        }
        return false;
    }

    Debug_Printf(LOG_ENGINE, "Texture built.");

    SetTextureFilter(r->font, TEXTURE_FILTER_POINT);

    r->glyphShader = LoadShader("assets/shaders/glyph.vs", "assets/shaders/glyph.fs");
    if (!IsShaderValid(r->glyphShader))
    {
        Debug_Printf(LOG_ENGINE, "Failed to build shader.");
        UnloadTexture(r->font);
        if (defaultCharset.pixels)
        {
            free(defaultCharset.pixels);
            defaultCharset.pixels = NULL;
        }
        return false;
    }

    Debug_Printf(LOG_ENGINE, "Shader built.");

    r->glyphShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(r->glyphShader, "mvp");
    r->glyphShader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(r->glyphShader, "texture0");
    SetShaderValueTexture(r->glyphShader, r->glyphShader.locs[SHADER_LOC_MAP_DIFFUSE], r->font);

    SetTargetFPS(60);

    r->src_w = defaultCharset.header.glyph_w;
    r->src_h = defaultCharset.header.glyph_h;

    r->scale = 1.0f;
    r->glyph_w = r->src_w;
    r->glyph_h = r->src_h;
    r->logical_w = BZZT_BOARD_DEFAULT_W * r->glyph_w;
    r->logical_h = BZZT_BOARD_DEFAULT_H * r->glyph_h;

    r->target = LoadRenderTexture(r->logical_w, r->logical_h);
    if (r->target.texture.id == 0)
    {
        Debug_Printf(LOG_ENGINE, "Failed to create render target.");
        if (IsShaderValid(r->glyphShader))
            UnloadShader(r->glyphShader);
        if (IsTextureValid(r->font))
            UnloadTexture(r->font);
        if (defaultCharset.pixels)
        {
            free(defaultCharset.pixels);
            defaultCharset.pixels = NULL;
        }
        return false;
    }

    SetTextureFilter(r->target.texture, TEXTURE_FILTER_POINT);
    renderer_logical_w = r->logical_w;
    renderer_logical_h = r->logical_h;
    refresh_present_rect(r);

    if (e && e->camera)
    {
        e->camera->cell_width = r->glyph_w;
        e->camera->cell_height = r->glyph_h;
    }

    Vector2 centerCoord = {(float)GetRenderWidth() / 2.0f, (float)GetRenderHeight() / 2.0f};
    r->centerCoord = centerCoord;
    return true;
}

Vector2 Renderer_ScreenToLogical(Vector2 screenPos)
{
    if (renderer_logical_w <= 0 || renderer_logical_h <= 0)
        return screenPos;

    Rectangle present_rect = window_output_rect(renderer_logical_w, renderer_logical_h);
    bool outside = screenPos.x < present_rect.x ||
                   screenPos.y < present_rect.y ||
                   screenPos.x >= present_rect.x + present_rect.width ||
                   screenPos.y >= present_rect.y + present_rect.height;

    if (outside)
        return (Vector2){-1.0f, -1.0f};

    return (Vector2){
        (screenPos.x - present_rect.x) * ((float)renderer_logical_w / present_rect.width),
        (screenPos.y - present_rect.y) * ((float)renderer_logical_h / present_rect.height)};
}

static Rectangle glyph_rec(Renderer *r, unsigned char ascii)
{
    int glyph = (int)ascii;
    if (glyph < 0 || glyph >= defaultCharset.header.glyph_count)
    {
        glyph = 0; // safe fallback
    }
    int col = glyph % 32;
    int row = glyph / 32;
    return (Rectangle){
        (float)(col * r->src_w),
        (float)(row * r->src_h),
        (float)r->src_w,
        (float)r->src_h};
}

void Renderer_Draw_Cell(Renderer *r, int cellX, int cellY, unsigned char glyph, Color_Bzzt fg, Color_Bzzt bg)
{
    // Convert to raylib Color
    Color rf = (Color){fg.r, fg.g, fg.b, 255};
    Color rb = (Color){bg.r, bg.g, bg.b, 255};

    Rectangle src = glyph_rec(r, glyph);
    Rectangle dst = {
        (float)(cellX * r->glyph_w),
        (float)(cellY * r->glyph_h),
        (float)r->glyph_w,
        (float)r->glyph_h};

    if (!(bg.r == COLOR_TRANSPARENT.r && bg.g == COLOR_TRANSPARENT.g && bg.b == COLOR_TRANSPARENT.b))
    {
        DrawRectangleRec(dst, rb);
    }
    DrawTexturePro(r->font, src, dst, (Vector2){0, 0}, 0.0f, rf);
}

void Renderer_Draw_Cell_Float(Renderer *r, float x, float y, unsigned char glyph,
                              Color_Bzzt fg, Color_Bzzt bg)
{
    // Convert to raylib Color
    Color rf = (Color){fg.r, fg.g, fg.b, 255};
    Color rb = (Color){bg.r, bg.g, bg.b, 255};

    Rectangle src = glyph_rec(r, glyph);

    Rectangle dst = {
        x * (float)r->glyph_w,
        y * (float)r->glyph_h,
        (float)r->glyph_w,
        (float)r->glyph_h};

    if (!(bg.r == COLOR_TRANSPARENT.r && bg.g == COLOR_TRANSPARENT.g &&
          bg.b == COLOR_TRANSPARENT.b))
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
    refresh_present_rect(r);

    BeginTextureMode(r->target);
    ClearBackground(BLACK);
    BeginShaderMode(r->glyphShader);
    switch (e->state)
    {
    case ENGINE_STATE_MENU:
        // if (e->world) Renderer_Draw_Board(r, e->world->boards[e->world->boards_current]);
        if (e->ui)
        {
            Renderer_Draw_UI(r, e->ui);
        }
        break;
    case ENGINE_STATE_TITLE:
    case ENGINE_STATE_PLAY:
        if (e->world)
            Renderer_Draw_Board(r, e->world, e->world->boards[e->world->boards_current]);
        if (e->ui)
            Renderer_Draw_UI(r, e->ui);
        break;
    case ENGINE_STATE_EDIT:
    {
        UI *ui = e->ui;
        if (ui)
        {
            Renderer_Draw_UI(r, ui);
        }
        draw_cursor(r, e);
        break;
    }
    default:
        break;
    }
    EndShaderMode();
    EndTextureMode();

    ClearBackground(BLACK);
    DrawTexturePro(r->target.texture,
                   (Rectangle){0.0f, 0.0f, (float)r->target.texture.width, (float)-r->target.texture.height},
                   r->present_rect,
                   (Vector2){0.0f, 0.0f},
                   0.0f,
                   WHITE);
}

void Renderer_Quit(Renderer *r)
{
    if (r->target.texture.id != 0)
        UnloadRenderTexture(r->target);
    if (IsTextureValid(r->font))
        UnloadTexture(r->font);
    if (IsShaderValid(r->glyphShader))
        UnloadShader(r->glyphShader);
    if (defaultCharset.pixels)
    {
        free(defaultCharset.pixels);
        defaultCharset.pixels = NULL;
    }
    renderer_logical_w = 0;
    renderer_logical_h = 0;
}
