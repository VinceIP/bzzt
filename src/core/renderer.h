#pragma once
#include <stdbool.h>
#include <raylib.h>
#include "color.h"

typedef struct UI UI;
typedef struct Engine Engine;
typedef struct Bzzt_Board Bzzt_Board;

typedef struct ZZTblock ZZTblock;

typedef struct Renderer
{
    int glyph_w, glyph_h; // rendered glyph size (in pixels)
    int src_w, src_h;     // source glyph size on texture
    float scale;          // scaling factor applied to glyphs
    Texture2D font;
    Vector2 centerCoord;
    Shader glyphShader;
} Renderer;

bool Renderer_Init(Renderer *, Engine *e, const char *fontPath);
void Renderer_Update(Renderer *r, Engine *e);
void Renderer_Draw_Cell(Renderer *r, int x, int y, unsigned char glyph,
                        Color_Bzzt fg, Color_Bzzt bg);
void Renderer_Draw_Cell_Float(Renderer *r, float x, float y, unsigned char glyph,
                              Color_Bzzt fg, Color_Bzzt bg);

void Renderer_Draw_UI(Renderer *r, const UI *ui);
void Renderer_Quit(Renderer *);