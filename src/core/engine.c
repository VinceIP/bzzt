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
#include <string.h>
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

#define ZZT_FILE "TOWN.ZZT" // temp file load

// Pressing p at zzt title screen
static void btn_title_press_play(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine || !ctx->engine->world)
        return;

    UIContext *ui_ctx = &ctx->engine->ui_ctx;
    if (!ui_ctx)
        return;

    ui_ctx->play_mode.overlay_play_screen_display->properties.enabled = true;
    ui_ctx->play_mode.overlay_play_screen_display->properties.visible = true;
    ui_ctx->title_mode.overlay_title_screen_display->properties.enabled = false;
    ui_ctx->title_mode.overlay_title_screen_display->properties.visible = false;

    Bzzt_World *world = ctx->engine->world;
    Bzzt_Board *start_board = world->start_board;
    Bzzt_Stat *player = start_board->stats[0];
    int idx = start_board->idx;
    Bzzt_World_Switch_Board_To(ctx->engine->world, idx, player->x, player->y);
    Bzzt_World_Set_Pause(world, true);
}

static void btn_toggle_quit(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine)
        return;

    int board_idx = ctx->engine->world->boards_current;
    if (board_idx == 0)
        strcpy(text_prompt->ud, "Quit to title?");
    else
        strcpy(text_prompt->ud, "End the game?");

    if (ctx->engine->world->on_title)
    {
        overlay_title_screen_display->properties.enabled = !overlay_title_screen_display->properties.enabled;
        overlay_title_screen_display->properties.visible = !overlay_title_screen_display->properties.visible;
    }
    else if (!ctx->engine->world->on_title)
    {
        overlay_play_screen_display->properties.enabled = !overlay_play_screen_display->properties.enabled;
        overlay_play_screen_display->properties.visible = !overlay_play_screen_display->properties.visible;
    }
    overlay_confirm_quit_buttons->properties.enabled = !overlay_confirm_quit_buttons->properties.enabled;
    overlay_confirm_quit_buttons->properties.visible = !overlay_confirm_quit_buttons->properties.visible;
}

static void btn_confirm_quit(UIActionContext *ctx)
{
    if (!ctx || !ctx->engine)
        return;

    int board_idx = ctx->engine->world->boards_current;
    if (board_idx == 0)
        Engine_Set_State(ctx->engine, ENGINE_STATE_MENU);
    else
    {
        // Change to title board
        Bzzt_Board *title_board = ctx->engine->world->boards[0];
        Bzzt_Stat *player = title_board->stats[0];
        Bzzt_World_Switch_Board_To(ctx->engine->world, 0, player->x, player->y);
        btn_toggle_quit(ctx); // And disable the prompt
    }
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

static bool verify_ui_components(EngineState state)
{
}

// Attempt to find all sidebar bui components used for zzt play mode
static bool register_ui_components(Engine *e)
{
    if (!e)
        return false;

    UI *ui = e->ui;
    if (!ui)
    {
        Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Tried to find UI components while no UI exists.");
        return false;
    }

    UIContext *ctx = e->ui_ctx;

    switch (e->state)
    {
    case ENGINE_STATE_MENU:
        break;

    case ENGINE_STATE_TITLE:
        ctx->title_mode.overlay_title_screen_display = UIOverlay_Find_By_Name(ui, "Title Screen Display");
        ctx->title_mode.overlay_confirm_quit_buttons = UIOverlay_Find_By_Name(ui, "confirm quit buttons");
        ctx->title_mode.text_prompt = UIElement_Find_By_Name(ui, "quit prompt text");
        break;

    case ENGINE_STATE_PLAY:
        ctx->play_mode.overlay_play_screen_display = UIOverlay_Find_By_Name(ui, "play screen display");
        ctx->play_mode.overlay_confirm_quit_buttons = UIOverlay_Find_By_Name(ui, "confirm quit buttons");
        ctx->play_mode.text_prompt = UIElement_Find_By_Name(ui, "quit prompt text");
        break;

    case ENGINE_STATE_EDIT:
        break;
    }

    text_prompt = UIElement_Find_By_Name(ui, "quit prompt text");

    return true;
}

static void register_ui_actions(Engine *e)
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
    char *file = ZZT_FILE;
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
        if (!register_ui_components(e->ui))
            Debug_Log(LOG_LEVEL_ERROR, LOG_UI, "Failed to find all sidebar UI components.");
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
    case ENGINE_STATE_MENU:
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
            {
                Editor_Destroy(e->editor);
                e->editor = NULL;
            }
        }

        if (e->world)
        {
            Bzzt_World_Destroy(e->world);
            e->world = NULL;
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
        if (e->editor)
        {
            Editor_Destroy(e->editor);
            e->editor = NULL;
        }
        zzt_title_init(e);
        break;
    }
}

static void sync_ui_to_world_state(Engine *e)
{
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
    register_ui_actions(e);
    e->ui_ctx = malloc(sizeof(UIContext));

    init_camera(e);

    // Populate empty charsets
    for (int i = 0; i < 8; ++i)
        e->charsets[i] = NULL;

    Engine_Set_State(e, ENGINE_STATE_MENU);
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
    case ENGINE_STATE_MENU:
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

    if (e->ui_ctx)
        free(e->ui_ctx);

    for (int i = 0; i < 8; ++i)
    {
        if (e->charsets[i] && e->charsets[i]->pixels)
        {
            free(e->charsets[i]->pixels);
        }
        free(e->charsets);
        e->charsets[i] = NULL;
    }
}