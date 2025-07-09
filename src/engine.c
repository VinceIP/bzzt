#include <stdbool.h>
#include "engine.h"
#include "input.h"
#include "color.h"

bool Engine_Init(Engine *e)
{
    if(!e) return false;

    e->state = SPLASH_MODE;
    e->running = true;
    e->debugShow = false;
    e->edit_mode_init_done = false;

    Cursor *c = &e->cursor;
    c->x = 0;
    c->y = 0;
    c->visible = true;
    c->enabled = true;
    c->blinkRate = 0.5;
    c->lastBlink = GetTime();
    c->glyph = '#';
    c->color = COLOR_WHITE;
    return true;
}

static void init_edit_mode(Engine *e)
{
    e->edit_mode_init_done = true;
    Cursor *c = &e->cursor;
    c->color = COLOR_WHITE;
    c->blinkRate = 0.5;
    c->glyph = '0';
    c->x = 50;
    c->y = 50;
    c->visible = true;
    c->enabled = true;
    c->lastBlink = GetTime();
}

void Engine_Update(Engine *e, InputState *in)
{
    switch (e->state)
    {
    case SPLASH_MODE:
        if (in->E_pressed)
            e->state = EDIT_MODE;
        break;

    case PLAY_MODE:
        break;

    case EDIT_MODE:
        if (in->Q_pressed)
            e->state = SPLASH_MODE;

        if (!e->edit_mode_init_done)
        {
            init_edit_mode(e);
        }

        break;

    default:
        break;
    }
}

void Engine_Quit(Engine *e)
{
    // Destroy renderer
    // Destroy world
}