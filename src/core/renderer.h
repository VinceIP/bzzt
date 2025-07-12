#pragma once
#include <stdbool.h>
#include <raylib.h>
#include "color.h"

typedef struct UI UI;
typedef struct Engine Engine;
typedef struct Board Board;

typedef struct Renderer
{
    int glyph_w, glyph_h, src_w, src_h;
    const char *inStr;
    Texture2D font;
    Vector2 centerCoord;
} Renderer;

bool Renderer_Init(Renderer *, const char *fontPath);
void Renderer_Update(Renderer *r, Engine *e);
void Renderer_Draw_Cell(Renderer *r, int x, int y, unsigned char glyph,
                        Color_Bzzt fg, Color_Bzzt bg);
void Renderer_Draw_Board(Renderer *, const Board *);
void Renderer_Draw_UI(Renderer *r, const UI *ui);
void Renderer_Quit(Renderer *)