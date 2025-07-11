#pragma once
#include <stdbool.h>
#include "raylib.h"
#include "color.h"
#include "world.h"
#include "engine.h"

typedef struct{
    uint8_t glyph;
    Color_Bzzt fg;
    Color_Bzzt bg;
} Cell;

typedef struct
{
    Texture2D font;
    int glyph_w, glyph_h;
    int src_w, src_h;
    Vector2 centerCoord;
    const char *inStr;
} Renderer;

bool Renderer_Init(Renderer *, const char *fontPath);
void Renderer_Update(Renderer *r, Engine *e);
void Renderer_DrawBoard(Renderer *, Board *);
void Renderer_Quit(Renderer *);