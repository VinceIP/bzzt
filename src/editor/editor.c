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

static UIOverlay *find_overlay_by_name(UI *ui, const char *name)
{
    if (!ui || !name)
        return NULL;

    for (int i = 0; i < ui->layer_count; ++i)
    {
        UILayer *layer = ui->layers[i];
        for (int j = 0; j < layer->surface_count; ++j)
        {
            UISurface *surface = layer->surfaces[j];
            for (int k = 0; k < surface->overlays_count; ++k)
            {
                UIOverlay *overlay = surface->overlays[k];
                if (overlay->properties.name && strcmp(overlay->properties.name, name) == 0)
                {
                    return overlay;
                }
            }
        }
    }
}

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
        UIOverlay *quitBox = find_overlay_by_name(e->ui, "quit dialog");
        UIOverlay *buttons = find_overlay_by_name(e->ui, "buttons");
        quitBox->properties.visible = true;
        buttons->properties.visible = false;
        // Engine_Set_State(e, ENGINE_STATE_SPLASH);
    }
}

void Editor_Update(Engine *e, InputState *in)
{
    handle_keys(e, in);
}
