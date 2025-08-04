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
    Cursor *c = &e->cursor;
    c->position = (Vector2){0, 0};
    c->visible = true;
    c->enabled = true;
    c->blinkRate = 0.5;
    c->lastBlink = GetTime();
    c->glyph = '#';
    c->color = COLOR_WHITE;
}

static void init_camera(Engine *e)
{
    e->camera = malloc(sizeof(Bzzt_Camera));
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
        Debug_Printf(LOG_ENGINE, "Error loading ZZT world %s", file);
    e->world->boards_current = 8;
}

bool Engine_Init(Engine *e)
{
    if (!e)
        return false;

    Debugger_Create();

    e->state = SPLASH_MODE;
    e->world = NULL;
    e->running = true;
    e->debugShow = false;
    e->edit_mode_init_done = false;

    init_cursor(e);

    e->ui = UI_Create(true, true);
    UI_Load_From_BUI(e->ui, "assets/ui/main_menu.bui");

    init_camera(e);

    // Populate empty charsets
    for (int i = 0; i < 8; ++i)
        e->charsets[i] = NULL;

    return true;
}

void Engine_Update(Engine *e, InputState *i, MouseState *m)
{
    Input_Poll(i);
    Mouse_Poll(m);

    if (e->cursor.enabled)
    {
        e->cursor.position = Handle_Cursor_Move(e->cursor.position, i, m, e->camera, e->camera->viewport.rect);
    }

    switch (e->state)
    {
    case SPLASH_MODE:
        Bzzt_World_Update(e->world, i);

        if (i->E_pressed)
        {
            e->state = EDIT_MODE;
            Editor_Init(e);
            if (e->world)
            {
                Bzzt_World *w = e->world;
                w->doUnload = true;
            }
            e->ui->visible = true;
        }
        else if (i->P_pressed)
        {
            e->state = PLAY_MODE;
            if (e->world)
            {
                e->world->doUnload = true;
            }
            play_init(e);
        }
        break;

    case PLAY_MODE:
        break;

    case EDIT_MODE:
        Editor_Update(e, i);
        if (i->Q_pressed)
        {
            e->state = SPLASH_MODE;
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
}