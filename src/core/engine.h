
#pragma once
#include "raylib.h"
#include "color.h"
#include "bz_char.h"

typedef struct Bzzt_World Bzzt_World;
typedef struct UI UI;
typedef struct Bzzt_Camera Bzzt_Camera;
typedef struct InputState InputState;
typedef struct MouseState MouseState;

typedef enum
{
    ENGINE_STATE_SPLASH,
    ENGINE_STATE_PLAY,
    ENGINE_STATE_EDIT
} EngineState;

typedef struct Cursor
{
    Vector2 position;
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
    EngineState state;
    UI *ui;
    Bzzt_World *world;
    Font font;
    Cursor *cursor;
    Bzzt_Camera *camera;
    BzztCharset *charsets[8]; // Loaded charsets (slot 0 = default)
    // Renderer renderer;
    // Input input;
    bool running;
    bool debugShow;
    bool edit_mode_init_done;
} Engine;

bool Engine_Init(Engine *);
void Engine_Update(Engine *, InputState *, MouseState *);
void Engine_Quit(Engine *);