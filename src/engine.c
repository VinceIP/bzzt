#include <stdbool.h>
#include "engine.h"
#include "input.h"

bool Engine_Init(Engine *e)
{
    return false;
}

void Engine_Update(Engine *e, InputState *in)
{
    switch (e->state)
    {
    case SPLASH_MODE:
        if(in->L_pressed) e->state = EDIT_MODE;
        break;

    case PLAY_MODE:
        break;

    case EDIT_MODE:
        if(in->Q_pressed) e->state = SPLASH_MODE;
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