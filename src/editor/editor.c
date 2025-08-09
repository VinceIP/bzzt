/**
 * @file editor.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "editor.h"
#include "engine.h"
#include "input.h"
#include "bzzt.h"
#include "ui.h"
#include "bui_loader.h"
#include "debugger.h"

int testX = 0;

#define CURSOR_BLINK_RATE 0.5

static void ui_init(Engine *e)
{
    if (!e)
        return;

    bool ok = UI_Load_From_BUI(e->ui, "assets/ui/sidebar_editor.bui");
    if (!ok)
        Debug_Printf(LOG_UI, "Editor failed to load sidebar.");
}

void Editor_Init(Engine *e)
{
    e->edit_mode_init_done = true;
    Cursor *c = e->cursor;
    c->color = COLOR_WHITE;
    c->blinkRate = CURSOR_BLINK_RATE;
    c->glyph = 219;
    c->position.x = 41;
    c->position.y = 14;
    c->visible = true;
    c->enabled = true;
    c->lastBlink = GetTime();

    e->camera->viewport.rect = (Rectangle){0, 0, 60, 25};

    ui_init(e);
}

static void handle_keys(Engine *e, InputState *in)
{
    if (in->Q_pressed || in->ESC_pressed)
    {
        UIOverlay *quitBox = UIOverlay_Find_By_Name(e->ui, "quit dialog");
        UIOverlay *buttons = UIOverlay_Find_By_Name(e->ui, "buttons");
        quitBox->properties.visible = true;
        buttons->properties.visible = false;
        // Engine_Set_State(e, ENGINE_STATE_SPLASH);
    }
}

void Editor_Update(Engine *e, InputState *in)
{
    handle_keys(e, in);
}
