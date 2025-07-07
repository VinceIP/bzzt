#pragma once
#include <stdbool.h>
#include "raylib.h"
#include "world.h"

typedef struct
{
    Texture2D font;
    int glyph_w, glyph_h;
} Renderer;

bool Renderer_Init(Renderer *, const char *fontPath);
void Renderer_DrawBoard(Renderer *, Board *);
void Renderer_Quit(Renderer *);