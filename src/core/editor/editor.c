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
#include <stdlib.h>
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

static UIElement_Text *text_elem;
static UIOverlay *overlay;
static char *prompt;

static void ui_init(Engine *e)
{
    if (!e)
        return;

    bool ok = UI_Load_From_BUI(e->ui, "assets/ui/sidebar_editor.bui");
    if (!ok)
        Debug_Printf(LOG_UI, "Editor failed to load sidebar.");
}

static void handle_keys(Engine *e, InputState *in)
{
    switch (e->editor->state)
    {
    case EDITOR_STATE_MAIN:
        if (in->Q_pressed || in->ESC_pressed)
        {
            UIOverlay *buttons = UIOverlay_Find_By_Name(e->ui, "buttons");
            buttons->properties.visible = false;
            UISurface *surface = UISurface_Find_By_Name(e->ui, "Sidebar");
            if (!surface)
                return;

            UIOverlay *overlay = UISurface_Add_New_Overlay(surface, "prompt", 0, 0, 0, 0, 25, 1, 0, true, true, LAYOUT_NONE, ANCHOR_TOP_LEFT, ALIGN_LEFT, 0);

            const char *prompt_str = "Really quit?\n(Y/N)";
            char *dup_prompt = strdup(prompt_str);
            UIElement_Text *prompt_text = UIText_Create(1, 4, COLOR_WHITE, COLOR_TRANSPARENT, false, ALIGN_LEFT, pass_through_caption, dup_prompt, true);
            UIOverlay_Add_New_Element(overlay, (UIElement *)prompt_text);

            e->editor->state = EDITOR_STATE_WAITING_FOR_KEY;
        }
        break;

    case EDITOR_STATE_WAITING_FOR_KEY:
        break;

    default:
        break;
    }
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

    e->editor = malloc(sizeof *e->editor);

    if (!e->editor)
    {
        Debug_Printf(LOG_EDITOR, "Error allocating editor to memory.");
        return;
    }

    e->editor->state = EDITOR_STATE_MAIN;

    ui_init(e);
}

void Editor_Update(Engine *e, InputState *in)
{
    handle_keys(e, in);
}

void Editor_Destroy(Engine *e)
{
    if (!e)
        return;

    if (!e->editor)
        return;

    free(e->editor);
    e->editor = NULL;
}
