#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "raylib.h"
#include "renderer.h"
#include "engine.h"
#include "color.h"
#include "bzzt.h"
#include "board_renderer.h"
#include "ui_renderer.h"
#include "bz_char.h"

#define TRANSPARENT_GLYPH 255

bool Renderer_Init(Renderer *r, Engine *e, const char *path)
{

    static BzztCharset defaultCharset;
    BZC_Load(path, &defaultCharset);
    e->charsets[0] = &defaultCharset;

    SetTextureFilter(r->font, TEXTURE_FILTER_POINT);
    SetTargetFPS(60);

    r->src_w = 9;
    r->src_h = 16;

    float desired = 2.75f;
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    if (BZZT_BOARD_DEFAULT_W * r->src_w * desired > screenW ||
        BZZT_BOARD_DEFAULT_H * r->src_h * desired > screenH)
    {
        float scaleW = (float)screenW / (BZZT_BOARD_DEFAULT_W * r->src_w);
        float scaleH = (float)screenH / (BZZT_BOARD_DEFAULT_H * r->src_h);
        desired = scaleW < scaleH ? scaleW : scaleH;
    }
    r->scale = desired;
    r->glyph_w = r->src_w * r->scale;
    r->glyph_h = r->src_h * r->scale;

    Vector2 centerCoord = {(float)GetRenderWidth() / 2, (float)GetRenderHeight() / 2};
    r->centerCoord = centerCoord;

    r->inStr = "\n\nP - play town.zzt\nL - load world\nE - editor\nESC - quit";

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

    // Fill bg
    DrawRectangleRec(dst, rb);

    // Draw fg
    DrawTexturePro(r->font, src, dst, (Vector2){0, 0}, 0.0f, rf);
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

static void draw_cursor(Renderer *r, Engine *e)
{
    Cursor *c = &e->cursor;
    double now = GetTime();

    if (now - c->lastBlink >= c->blinkRate)
    {
        c->visible = !c->visible;
        c->lastBlink = now;
    }

    if (c->visible)
    {
        Renderer_Draw_Cell(r, c->position.x, c->position.y, c->glyph, c->color, COLOR_BLACK);
    }
}

void Renderer_Update(Renderer *r, Engine *e)
{
    ClearBackground(BLACK);
    switch (e->state)
    {
    case SPLASH_MODE:
        Renderer_Draw_Board(r, e->world->boards[e->world->boards_current]);
        draw_splash(r, e);
        break;

    case PLAY_MODE:
        Renderer_Draw_Board(r, e->world->boards[e->world->boards_current]);
        break;

    case EDIT_MODE:
        UI *ui = e->ui;
        if (ui)
        {
            Renderer_Draw_UI(r, ui);
        }
        else
        {
            fprintf(stderr, "Failed to draw UI.");
        }
        draw_cursor(r, e);
        break;

    default:
        break;
    }
}

void Renderer_Quit(Renderer *r)
{
    UnloadTexture(r->font);
}
