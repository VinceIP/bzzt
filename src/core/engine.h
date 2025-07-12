
#pragma once
#include "raylib.h"
#include "color.h"

typedef struct World World;
typedef struct UI UI;
typedef struct BzztCamera BzztCamera;
typedef struct InputState InputState;

typedef enum
{
    SPLASH_MODE,
    PLAY_MODE,
    EDIT_MODE
} EState;

typedef struct Cursor
{
    int x, y;
    bool visible;
    bool enabled;
    double lastBlink;
    double blinkRate;
    int paletteIndex;
    enum Tool
    {
        BRUSH,
        FILL,
        SELECT
    } tool;
    unsigned char glyph;
    Color_Bzzt color;
} Cursor;

typedef struct Engine
{
    EState state;
    UI *ui;
    World *world;
    Font font;
    Cursor cursor;
    BzztCamera *camera;
    // Renderer renderer;
    // Input input;
    bool running;
    bool debugShow;
    bool edit_mode_init_done;
} Engine;

bool Engine_Init(Engine *);
void Engine_Update(Engine *, InputState *);
void Engine_Quit(Engine *);