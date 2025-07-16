#include <stdbool.h>
#include <stdlib.h>
#include "debugger.h"
#include "engine.h"
#include "world.h"
#include "input.h"
#include "editor.h"
#include "ui.h"
#include "ui_surface.h"
#include "camera.h"
#include "color.h"
#include "coords.h"

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
    cam->viewport.rect = (Rectangle){0, 0, 80,25}; //remove magic nums
    cam->rect = cam->viewport.rect;
    cam->cell_width = 16;
    cam->cell_height = 32;
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

    e->ui = UI_Create();

    init_camera(e);

    return true;
}

void Engine_Update(Engine *e, InputState *i, MouseState *m)
{
    Input_Poll(i);
    Mouse_Poll(m);

    if(e->cursor.enabled){
        Vector2 cell = Camera_ScreenToCell(e->camera, m->screenPosition);
        Debug_Printf(LOG_ENGINE, "Mouse in cell: %d,%d", cell.x, cell.y);
        Debug_Printf(LOG_ENGINE, "Mouse screen: %d,%d", m->screenPosition.x, m->screenPosition.y);
        if(cell.x >0){
            e->cursor.x = cell.x;
            e->cursor.y = cell.y;
        }
    }

    switch (e->state)
    {
    case SPLASH_MODE:
        World_Update(e->world, i);

        if (i->E_pressed)
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
        Editor_Update(e, i);
        if (i->Q_pressed)
        {
            e->state = SPLASH_MODE;
            e->world = World_Create("Title Screen");
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