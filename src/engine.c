#include <stdbool.h>
#include "engine.h"
#include "input.h"
#include "color.h"

bool Engine_Init(Engine *e)
{
    return false;
}

static void init_edit_mode(Engine *e)
{
    e->edit_mode_init_done = true;
    Cursor *c = &e->cursor;
    c->color = COLOR_WHITE;
    c->blinkRate = 8.0;
    c->glyph = '#';
    c->x = 25;
    c->y = 25;
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