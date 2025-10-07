/**
 * @file engine.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "raylib.h"
#include "color.h"
#include "bz_char.h"

typedef struct Bzzt_World Bzzt_World;
typedef struct UI UI;
typedef struct Bzzt_Camera Bzzt_Camera;
typedef struct InputState InputState;
typedef struct MouseState MouseState;
typedef struct Editor Editor;
typedef struct UIActionRegistry UIActionRegistry;
typedef struct UIOverlay UIOverlay;
typedef struct UIElement UIElement;
typedef struct UIElement_Text UIElement_Text;

typedef enum EngineState
{
    ENGINE_STATE_MENU,  // Main menu to access play/edit modes or quit
    ENGINE_STATE_TITLE, // At title screen board of a zzt world
    ENGINE_STATE_PLAY,  // Playing a zzt world
    ENGINE_STATE_EDIT,  // Using the editor
    ENGINE_STATE__COUNT
} EngineState;

// Holds the cursor used in the editor
typedef struct Cursor
{
    Vector2 position;
    bool visible;
    bool enabled;
    double lastBlink;
    double blinkRate;
    int paletteIndex;
    enum Tool
    {
        BRUSH,
        FILL,
        SELECT
    } tool;
    unsigned char glyph;
    Color_Bzzt color;
} Cursor;

// Holds references to UI components used by various bzzt engine states
typedef struct UIContext
{

    struct
    {

    } main_menu;

    struct
    {
        UIOverlay *overlay_title_screen_display;
        UIOverlay *overlay_confirm_quit_buttons;
        UIElement_Text *text_prompt; // 1-line text elem that holds quit/save/etc prompts

        bool components_found;
    } title_mode;

    struct
    {
        UIOverlay *overlay_play_screen_display; // Overlay containing all gameplay buttons and info
        UIOverlay *overlay_confirm_quit_buttons;
        UIElement_Text *text_prompt; // 1-line text elem that holds quit/save/etc prompts

        bool components_found; // true if all components were found in the .bui
    } play_mode;

} UIContext;

// Main data container of the bzzt engine
typedef struct Engine
{
    EngineState state;
    EngineState pending_state;
    UI *ui;
    Editor *editor;
    Bzzt_World *world;
    Font font;
    Cursor *cursor;
    InputState *input;
    Bzzt_Camera *camera;
    BzztCharset *charsets[8]; // Loaded charsets (slot 0 = default)

    UIActionRegistry *action_registry; // Contains pointers to various functions called on UI button presses
    UIContext *ui_ctx;                 // Exposes UI components used by core engine

    bool running;
    bool firstBoot;
    bool debugShow;
    bool edit_mode_init_done;
    bool has_pending_state;
} Engine;

bool Engine_Init(Engine *e, InputState *in);
void Engine_Update(Engine *, InputState *, MouseState *);
void Engine_Quit(Engine *);
void Engine_Set_State(Engine *e, EngineState next);