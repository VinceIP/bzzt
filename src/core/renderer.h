#pragma once
#include <stdbool.h>
#include "color.h"

struct Texture2D;
struct Vector2;
typedef struct UI UI;
typedef struct Engine Engine;
typedef struct Board Board;

typedef struct Renderer
{
    Texture2D font;
    int glyph_w, glyph_h;
    int src_w, src_h;
    Vector2 centerCoord;
    const char *inStr;
} Renderer;

bool Renderer_Init(Renderer *, const char *fontPath);
void Renderer_Update(Renderer *r, Engine *e);
void Renderer_Draw_Board(Renderer *, Board *);
void Renderer_Draw_UI(Renderer *r, UI *ui);
void Renderer_Quit(Renderer *);