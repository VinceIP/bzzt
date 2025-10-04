/**
 * @file engine.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-07
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include "debugger.h"
#include "engine.h"
#include "input.h"
#include "editor.h"
#include "ui.h"
#include "bui_loader.h"
#include "color.h"
#include "coords.h"
#include "bzzt.h"
#include "raylib.h"

static void btn_title_press_play(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine || !ctx->engine->world)
        return;

    Debug_Printf(LOG_ENGINE, "pressed");
    Bzzt_World *world = ctx->engine->world;
    Bzzt_Board *start_board = world->start_board;
    Bzzt_Stat *player = start_board->stats[0];
    int idx = start_board->idx;
    Debug_Printf(LOG_ENGINE, "idx: %d", idx);

    Bzzt_World_Switch_Board_To(ctx->engine->world, idx, player->x, player->y);
}

static void btn_toggle_quit(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine)
        return;

    UIOverlay *overlay_buttons = UIOverlay_Find_By_Name(ctx->engine->ui, "Title Screen Buttons");
    UIOverlay *overlay_confirm_quit_buttons = UIOverlay_Find_By_Name(ctx->engine->ui, "Title Confirm Quit");
    if (overlay_buttons && overlay_confirm_quit_buttons)
    {
        overlay_buttons->properties.enabled = !overlay_buttons->properties.enabled;
        overlay_buttons->properties.visible = !overlay_buttons->properties.visible;
        overlay_confirm_quit_buttons->properties.enabled = !overlay_confirm_quit_buttons->properties.enabled;
        overlay_confirm_quit_buttons->properties.visible = !overlay_confirm_quit_buttons->properties.visible;
    }
}

static void btn_confirm_quit(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine)
        return;

    Engine_Set_State(ctx->engine, ENGINE_STATE_SPLASH);
}

static void splash_press_play(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine)
        return;
    Engine_Set_State(ctx->engine, ENGINE_STATE_PLAY);
}

static void splash_press_quit(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine)
        return;
    Debug_Log(LOG_LEVEL_DEBUG, LOG_ENGINE, "Quitting");
    ctx->engine->input->quit = true;
}

static void engine_register_actions(Engine *e)
{
    if (!e || !e->action_registry)
        return;
    Debug_Log(LOG_LEVEL_DEBUG, LOG_UI, "Registering engine UI actions...");

    UIAction_Register(e->action_registry, "btn_confirm_quit", btn_confirm_quit);
    UIAction_Register(e->action_registry, "btn_toggle_quit", btn_toggle_quit);
    UIAction_Register(e->action_registry, "btn_title_press_play", btn_title_press_play);
    UIAction_Register(e->action_registry, "splash_press_play", splash_press_play);
    UIAction_Register(e->action_registry, "splash_press_quit", splash_press_quit);
}

static void load_splash_screen(UI *ui, UIActionRegistry *registry);

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

static void zzt_title_init(Engine *e)
{
    char *file = "TOWN.ZZT";
    e->world = Bzzt_World_From_ZZT_World(file);

    if (!e->world)
    {
        Debug_Printf(LOG_ENGINE, "Error loading ZZT world %s", file);
        return;
    }

    if (e->ui)
    {
        UI_Destroy(e->ui);
        e->ui = UI_Create(true, true);
        UI_Load_From_BUI(e->ui, "assets/ui/sidebar_play.bui");
        UI_Resolve_Button_Actions(e->ui, e->action_registry);
        UI_Reset_Button_State();
    }
}

static void load_splash_screen(UI *ui, UIActionRegistry *registry)
{
    if (!ui)
    {
        Debug_Printf(LOG_UI, "Engine UI creation failed.");
        return;
    }

    bool ok = UI_Load_From_BUI(ui, "assets/ui/main_menu.bui");
    if (!ok)
    {
        Debug_Printf(LOG_ENGINE, "Engine failed in loading main menu from .bui.");
        return;
    }
    else
    {
        Debug_Printf(LOG_ENGINE, "Engine reports success loading UI from .bui.");
        UI_Resolve_Button_Actions(ui, registry);
    }
}

static void apply_state_change(Engine *e)
{
    if (!e || !e->has_pending_state)
        return;

    e->has_pending_state = false;
    EngineState next = e->pending_state;
    e->state = next;

    switch (next)
    {
    case ENGINE_STATE_SPLASH:
        if (!e->firstBoot) // Only refresh the UI if the engine has not just been initialized
        {
            if (e->ui)
                UI_Destroy(e->ui);
            e->ui = UI_Create(true, true);
            if (!e->ui)
            {
                Debug_Log(LOG_LEVEL_ERROR, LOG_UI, "Failed creating new UI after state change.");
                return;
            }

            if (e->editor)
                Editor_Destroy(e->editor);
        }

        load_splash_screen(e->ui, e->action_registry);
        break;

    case ENGINE_STATE_EDIT:
        if (e->ui)
            UI_Destroy(e->ui);
        e->ui = UI_Create(true, true);
        Editor_Init(e);
        if (e->world)
        {
            Bzzt_World *w = e->world;
            w->doUnload = true;
        }
        break;

    case ENGINE_STATE_PLAY:
        if (e->world)
            e->world->doUnload;
        if (e->editor)
        {
            Editor_Destroy(e->editor);
            e->editor = NULL;
        }
        zzt_title_init(e);
        break;
    }
}

void Engine_Set_State(Engine *e, EngineState next)
{
    if (!e)
        return;

    e->pending_state = next;
    e->has_pending_state = true;
    Debug_Log(LOG_LEVEL_DEBUG, LOG_ENGINE, "State change queued: %d -> %d", e->state, e->pending_state);
}

bool Engine_Init(Engine *e, InputState *in)
{
    if (!e || !in)
        return false;

    Debugger_Create();
    Debug_Printf(LOG_ENGINE, "Initializing bzzt engine.");

    e->world = NULL;
    e->editor = NULL;
    e->running = true;
    e->debugShow = false;
    e->edit_mode_init_done = false;
    e->input = in;
    e->firstBoot = true;
    e->has_pending_state = false;

    init_cursor(e);

    e->ui = UI_Create(true, true);
    e->action_registry = UIAction_Registry_Create();
    engine_register_actions(e);

    init_camera(e);

    // Populate empty charsets
    for (int i = 0; i < 8; ++i)
        e->charsets[i] = NULL;

    Engine_Set_State(e, ENGINE_STATE_SPLASH);
    apply_state_change(e);

    e->firstBoot = false;
    return true;
}

void Engine_Update(Engine *e, InputState *i, MouseState *m)
{
    Input_Poll(i);
    Mouse_Poll(m);
    if (e->cursor && e->cursor->enabled)
    {
        e->cursor->position = Handle_Cursor_Move(e->cursor->position, i, m, e->camera, e->camera->viewport.rect);
    }

    if (e->ui)
        UI_Update_Button_Events(e->ui, e);

    switch (e->state)
    {
    case ENGINE_STATE_SPLASH:
        break;

    case ENGINE_STATE_PLAY:
        if (e->world)
        {
            Bzzt_World_Update(e->world, i);
        }
        break;

    case ENGINE_STATE_EDIT:
        Editor_Update(e, i);
        break;

    default:
        break;
    }

    apply_state_change(e);
}

void Engine_Quit(Engine *e)
{
    if (!e)
        return;
    if (e->world)
        Bzzt_World_Destroy(e->world);

    if (e->camera)
        free(e->camera);

    if (e->cursor)
        free(e->cursor);

    if (e->editor)
    {
        Editor_Destroy(e->editor);
    }

    if (e->ui)
        UI_Destroy(e->ui);

    if (e->action_registry)
        UIAction_Registry_Destroy(e->action_registry);

    for (int i = 0; i < 8; ++i)
    {
        if (e->charsets[i] && e->charsets[i]->pixels)
        {
            free(e->charsets[i]->pixels);
        }
    }
}