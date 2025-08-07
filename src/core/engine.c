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

static void change_state(Engine *e, EngineState target_state)
{
    switch (target_state)
    {
    case ENGINE_STATE_SPLASH:
        break;

    case ENGINE_STATE_EDIT:
        e->state = ENGINE_STATE_EDIT;
        if (e->ui)
        {
            fprintf(stderr, "Destroying UI\n");
            UI_Destroy(e->ui);
        }
        e->ui = UI_Create(true, true);
        Editor_Init(e);
        if (e->world)
        {
            Bzzt_World *w = e->world;
            w->doUnload = true;
        }

        e->ui->visible = true;
        break;

    case ENGINE_STATE_PLAY:
        break;
    }
}

bool Engine_Init(Engine *e)
{
    if (!e)
        return false;

    Debugger_Create();
    Debug_Printf(LOG_ENGINE, "Initializing bzzt engine.");

    e->state = ENGINE_STATE_SPLASH;
    e->world = NULL;
    e->running = true;
    e->debugShow = false;
    e->edit_mode_init_done = false;

    init_cursor(e);

    e->ui = UI_Create(true, true);
    if (!e->ui)
    {
        Debug_Printf(LOG_UI, "Engine UI creation failed.");
        return false;
    }

    bool ok = UI_Load_From_BUI(e->ui, "assets/ui/main_menu.bui");
    if (!ok)
    {
        Debug_Printf(LOG_ENGINE, "Engine failed in loading main menu from .bui.");
        return false;
    }
    else
        Debug_Printf(LOG_ENGINE, "Engine reports success loading UI from .bui.");

    init_camera(e);

    // Populate empty charsets
    for (int i = 0; i < 8; ++i)
        e->charsets[i] = NULL;

    return true;
}

void Engine_Update(Engine *e, InputState *i, MouseState *m)
{
    puts("updating engine\n");
    Input_Poll(i);
    Mouse_Poll(m);

    if (e->cursor && e->cursor->enabled)
    {
        e->cursor->position = Handle_Cursor_Move(e->cursor->position, i, m, e->camera, e->camera->viewport.rect);
    }

    switch (e->state)
    {
    case ENGINE_STATE_SPLASH:
        if (i->E_pressed)
        {
            change_state(e, ENGINE_STATE_EDIT);
        }
        else if (i->P_pressed)
        {
            e->state = ENGINE_STATE_PLAY;
            if (e->world)
            {
                e->world->doUnload = true;
            }
            play_init(e);
        }
        break;

    case ENGINE_STATE_PLAY:
        break;

    case ENGINE_STATE_EDIT:
        Editor_Update(e, i);
        if (i->Q_pressed)
        {
            e->state = ENGINE_STATE_SPLASH;
            e->world = Bzzt_World_Create("Title Screen");
        }

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
        Bzzt_World_Unload(e->world);
        e->world = NULL;
    }

    if (e->camera)
    {
        free(e->camera);
        e->camera = NULL;
    }

    if (e->ui)
    {
        UI_Destroy(e->ui);
        e->ui = NULL;
    }

    for (int i = 0; i < 8; ++i)
    {
        if (e->charsets[i] && e->charsets[i]->pixels)
        {
            free(e->charsets[i]->pixels);
            e->charsets[i]->pixels = NULL;
        }
    }
}