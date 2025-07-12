#include <stdbool.h>
#include <stdlib.h>
#include "engine.h"
#include "world.h"
#include "input.h"
#include "editor.h"
#include "ui.h"
#include "ui_surface.h"
#include "camera.h"
#include "color.h"

static void init_cursor(Engine *e)
{
    Cursor *c = &e->cursor;
    c->x = 0;
    c->y = 0;
    c->visible = true;
    c->enabled = true;
    c->blinkRate = 0.5;
    c->lastBlink = GetTime();
    c->glyph = '#';
    c->color = COLOR_WHITE;
}

static void init_camera(Engine *e)
{
    e->camera = malloc(sizeof(BzztCamera));
    BzztCamera *cam = e->camera;
    cam->viewport.rect = (Rectangle){0, 0, 0, 0};
}

bool Engine_Init(Engine *e)
{
    if (!e)
        return false;

    e->state = SPLASH_MODE;
    e->world = NULL;
    e->running = true;
    e->debugShow = false;
    e->edit_mode_init_done = false;

    init_cursor(e);

    e->ui = UI_Create();

    init_camera(e);

    return true;
}

void Engine_Update(Engine *e, InputState *in)
{
    Input_Poll(in);
    switch (e->state)
    {
    case SPLASH_MODE:
        World_Update(e->world, in);

        if (in->E_pressed)
        {
            e->state = EDIT_MODE;
            Editor_Init(e);
            if (e->world)
            {
                World *w = e->world;
                w->doUnload = true;
            }
            e->ui->visible = true;
        }
        break;

    case PLAY_MODE:
        break;

    case EDIT_MODE:
        Editor_Update(e, in);
        if (in->Q_pressed)
        {
            e->state = SPLASH_MODE;
            e->world = world_create("Title Screen");
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
        World_Unload(e->world);
        e->world = NULL;
    }
}