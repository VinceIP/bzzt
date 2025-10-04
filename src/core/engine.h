/**
 * @file engine.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "raylib.h"
#include "color.h"
#include "bz_char.h"

typedef struct Bzzt_World Bzzt_World;
typedef struct UI UI;
typedef struct Bzzt_Camera Bzzt_Camera;
typedef struct InputState InputState;
typedef struct MouseState MouseState;
typedef struct Editor Editor;
typedef struct UIActionRegistry UIActionRegistry;

typedef enum EngineState
{
    ENGINE_STATE_SPLASH,
    ENGINE_STATE_PLAY,
    ENGINE_STATE_EDIT,
    ENGINE_STATE__COUNT
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
    EngineState pending_state;
    UI *ui;
    Editor *editor;
    Bzzt_World *world;
    Font font;
    Cursor *cursor;
    InputState *input;
    Bzzt_Camera *camera;
    BzztCharset *charsets[8]; // Loaded charsets (slot 0 = default)
    UIActionRegistry *action_registry;
    // Renderer renderer;
    // Input input;
    bool running;
    bool firstBoot;
    bool debugShow;
    bool edit_mode_init_done;
    bool has_pending_state;
} Engine;

bool Engine_Init(Engine *e, InputState *in);
void Engine_Update(Engine *, InputState *, MouseState *);
void Engine_Quit(Engine *);
void Engine_Set_State(Engine *e, EngineState next);