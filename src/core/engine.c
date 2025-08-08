/**
 * @file engine.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-07
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include "debugger.h"
#include "engine.h"
#include "input.h"
#include "editor.h"
#include "ui.h"
#include "yaml_loader.h"
#include "color.h"
#include "coords.h"
#include "bzzt.h"
#include "raylib.h"

// Handle keyboard input on splash screen
static void splash_key_handler(Engine *e);
// Handle keyboard input in play mode
static void play_key_handler(Engine *e);
// Handle keyboard input in editor mode
static void editor_key_handler(Engine *e);

static void load_splash_screen(UI *ui);

static const Key_Handler STATE_KEY_FN[ENGINE_STATE__COUNT] = {
    [ENGINE_STATE_SPLASH] = splash_key_handler,
    [ENGINE_STATE_PLAY] = play_key_handler,
    [ENGINE_STATE_EDIT] = editor_key_handler};

static void init_cursor(Engine *e)
{
    Debug_Printf(LOG_ENGINE, "Initializing cursor.");
    Cursor *c = malloc(sizeof(Cursor));
    if (!c)
    {
        Debug_Printf(LOG_ENGINE, "Error allocating cursor.");
        return;
    }
    c->position = (Vector2){0, 0};
    c->visible = true;
    c->enabled = true;
    c->blinkRate = 0.5;
    c->lastBlink = GetTime();
    c->glyph = '#';
    c->color = COLOR_WHITE;

    e->cursor = c;
}

static void init_camera(Engine *e)
{
    if (!e)
        return;
    Debug_Printf(LOG_ENGINE, "Initializing camera.");
    e->camera = malloc(sizeof(Bzzt_Camera));
    if (!e->camera)
    {
        Debug_Printf(LOG_ENGINE, "Engine failed to allocate camera.");
        return;
    }
    e->camera->viewport.rect = (Rectangle){0, 0, 80, 25}; // remove magic nums
    e->camera->rect = e->camera->viewport.rect;
    e->camera->cell_width = 16;
    e->camera->cell_height = 32;
}

static void play_init(Engine *e)
{
    char *file = "burglar1.zzt";
    e->world = Bzzt_World_From_ZZT_World(file);
    if (!e->world)
    {
        Debug_Printf(LOG_ENGINE, "Error loading ZZT world %s", file);
        return;
    }
    e->world->boards_current = 8;
}

static void load_splash_screen(UI *ui)
{
    if (!ui)
    {
        Debug_Printf(LOG_UI, "Engine UI creation failed.");
        return;
    }

    bool ok = UI_Load_From_BUI(ui, "assets/ui/main_menu.bui");
    if (!ok)
    {
        Debug_Printf(LOG_ENGINE, "Engine failed in loading main menu from .bui.");
        return;
    }
    else
        Debug_Printf(LOG_ENGINE, "Engine reports success loading UI from .bui.");
}

void Engine_Set_State(Engine *e, EngineState next)
{
    if (!e)
        return;

    // Change state and setup new input handler function
    e->state = next;
    Input_Set_Handler(e->input, STATE_KEY_FN[next]);

    // Handle specific setups
    switch (next)
    {
    case ENGINE_STATE_SPLASH:
        if (!e->firstBoot) // Only refresh the UI if the engine has not just been initialized
        {
            if (e->ui)
                UI_Destroy(e->ui);
            e->ui = UI_Create(true, true);
        }
        load_splash_screen(e->ui);
        break;

    case ENGINE_STATE_EDIT:
        if (e->ui)
        {
            UI_Destroy(e->ui);
        }
        e->ui = UI_Create(true, true);
        Editor_Init(e);
        if (e->world)
        {
            Bzzt_World *w = e->world;
            w->doUnload = true;
        }
        break;

    case ENGINE_STATE_PLAY:
        if (e->world)
        {
            e->world->doUnload = true;
        }
        play_init(e);
        break;
    }
}

bool Engine_Init(Engine *e, InputState *in)
{
    if (!e || !in)
        return false;

    Debugger_Create();
    Debug_Printf(LOG_ENGINE, "Initializing bzzt engine.");

    e->world = NULL;
    e->running = true;
    e->debugShow = false;
    e->edit_mode_init_done = false;
    e->input = in;
    e->firstBoot = true;

    init_cursor(e);

    e->ui = UI_Create(true, true);
    load_splash_screen(e->ui);

    init_camera(e);

    // Populate empty charsets
    for (int i = 0; i < 8; ++i)
        e->charsets[i] = NULL;

    Engine_Set_State(e, ENGINE_STATE_SPLASH);

    e->firstBoot = false;
    return true;
}

void Engine_Update(Engine *e, InputState *i, MouseState *m)
{
    Input_Poll(i);
    Mouse_Poll(m);
    if (e->cursor && e->cursor->enabled)
    {
        e->cursor->position = Handle_Cursor_Move(e->cursor->position, i, m, e->camera, e->camera->viewport.rect);
    }

    if (i->key_handler)
        i->key_handler(e);

    switch (e->state)
    {
    case ENGINE_STATE_SPLASH:
        break;

    case ENGINE_STATE_PLAY:
        break;

    case ENGINE_STATE_EDIT:
        Editor_Update(e, i);
        break;

    default:
        break;
    }
}

void Engine_Quit(Engine *e)
{
    if (!e)
        return;
    if (e->world)
    {
        Bzzt_World_Destroy(e->world);
    }

    if (e->camera)
    {
        free(e->camera);
    }

    if (e->cursor)
    {
        free(e->cursor);
    }

    if (e->ui)
    {
        UI_Destroy(e->ui);
    }

    for (int i = 0; i < 8; ++i)
    {
        if (e->charsets[i] && e->charsets[i]->pixels)
        {
            free(e->charsets[i]->pixels);
        }
    }
}

static void splash_key_handler(Engine *e)
{
    InputState *i = e->input;

    if (i->E_pressed)
        Engine_Set_State(e, ENGINE_STATE_EDIT);
    else if (i->P_pressed)
    {
        Engine_Set_State(e, ENGINE_STATE_PLAY);
    }
}

static void play_key_handler(Engine *e)
{
    // Stub
    return;
}

static void editor_key_handler(Engine *e)
{
    InputState *i = e->input;

    if (i->Q_pressed)
    {
        Engine_Set_State(e, ENGINE_STATE_SPLASH);
    }
}
