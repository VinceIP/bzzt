/**
 * @file ui.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.2
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "color.h"

struct Color;
typedef struct Renderer Renderer;
typedef struct Bzzt_Tile Bzzt_Tile;
typedef struct cJSON cJSON;
typedef struct UISurface UISurface;
typedef struct UIOverlay UIOverlay;
typedef struct UIButton UIButton;

typedef enum ElementType
{
    UI_ELEM_NONE,
    UI_ELEM_TEXT,
    UI_ELEM_BUTTON,
    UI_ELEM_INPUT
} ElementType;

typedef enum UILayout
{
    LAYOUT_NONE,
    LAYOUT_VBOX,
    LAYOUT_HBOX
} UILayout;

typedef enum UIAlign
{
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} UIAlign;

typedef enum UIAnchor
{
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_CENTER,
    ANCHOR_TOP_RIGHT,
    ANCHOR_MIDDLE_LEFT,
    ANCHOR_MIDDLE_CENTER,
    ANCHOR_MIDDLE_RIGHT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_CENTER,
    ANCHOR_BOTTOM_RIGHT
} UIAnchor;

typedef struct PlaysciiAsset
{
    int width, height;
    uint32_t *glyphs;
    uint8_t *fg, *bg;
    Color palette[32];
} PlaysciiAsset;

typedef enum
{
    BIND_INT,
    BIND_INT16,
    BIND_FLOAT,
    BIND_STR
} BindType;

typedef enum
{
    UI_EVENT_DOWN,
    UI_EVENT_PRESS,
    UI_EVENT_RELEASE
} UIEventType;

typedef struct
{
    enum
    {
        KEYBOARD,
        MOUSE,
        GAMEPAD
    } type;
    int code;
} UIInputBinding;

// Holds all possible input codes for a button, max of 8
typedef struct
{
    UIInputBinding bindings[8];
    int count;
} UIInputBindingSet;

#define UI_ACTION_REGISTRY_MAX 256

typedef struct
{
    const void *ptr;
    const char *fmt;
    BindType type;
    char buf[32];
    bool on_heap;
} TextBinding;

typedef void (*UIButtonAction)(void *ud);

// Properties shared by most UI components
typedef struct UIProperties
{
    char *name;
    int id;
    int x, y, z, w, h;
    int padding;
    bool visible;
    bool enabled;
    bool expand;
    UIAlign align;
    void *parent;
} UIProperties;

// Base struct of UI elements
typedef struct UIElement
{
    ElementType type;
    UIProperties properties;
    Bzzt_Tile *cells;
    int cell_count;
    void *child;
} UIElement;

// A UI text field
typedef struct UIElement_Text
{
    UIElement base;
    const char *(*textCallback)(void *ud);
    void *ud;
    Color_Bzzt fg, bg;
    bool wrap;
    bool owns_ud;
} UIElement_Text;

// Exposes UI and engine features to UI actions
typedef struct Engine Engine;
typedef struct
{
    Engine *engine;
    UIElement *element;
    UIButton *button;
    UIEventType event_type; // Which type of event triggered this
    void *user_data;        // Optional custom data
} UIActionContext;

typedef struct
{
    enum
    {
        C_FUNCTION,
        BOOP_FUNCTION
    } type; // Whether an action is bound to an internal C function of a boop function

    union
    {
        void (*c_func)(UIActionContext *ctx);
        struct
        {
            void *script_vm;
            void *function_ref;
        } boop_func; // tbd
    } handler;
} UIActionHandler;

// Stores string names from .bui and resolved handlers for event types
typedef struct
{
    char *on_down_action;
    char *on_press_action;
    char *on_release_action;

    UIActionHandler on_down_handler;
    UIActionHandler on_press_handler;
    UIActionHandler on_release_handler;
} UIButtonEvents;

// Global lookup table mapping action name strings from bui to
// executable handlers.
typedef struct UIActionRegistry
{
    char *action_names[UI_ACTION_REGISTRY_MAX];
    UIActionHandler handlers[UI_ACTION_REGISTRY_MAX];
    int count;
} UIActionRegistry;

typedef struct UIButton
{
    UIElement base;
    UIButtonAction onClick;
    void *ud;
    UIElement_Text *label;

    UIInputBindingSet input_bindings;
    UIButtonEvents events;
} UIButton;

typedef struct UISurface
{
    UIProperties properties;
    Bzzt_Tile *cells;
    int cell_count;
    UIOverlay **overlays;
    int overlays_count, overlays_cap;
} UISurface;

typedef struct UIOverlay
{
    UISurface *surface; // parent surface
    UIProperties properties;
    UIElement **elements;
    int elements_count, elements_cap;
    UILayout layout;
    UIAnchor anchor;
    UIAlign align;
    int spacing;
} UIOverlay;

typedef struct UILayer
{
    char *name;
    int id;
    bool visible, enabled;
    int index;
    UISurface **surfaces;
    int surface_count, surface_cap;
} UILayer;

typedef struct UI
{
    bool visible, enabled;
    UILayer **layers;
    int layer_count, layer_cap;
} UI;

UI *UI_Create(bool visible, bool enabled);
void UI_Update(UI *ui);
void UI_Destroy(UI *ui);

UILayer *UI_Add_New_Layer(UI *ui, bool visible, bool enabled);
void UI_Add_Surface(UI *ui, int targetIndex, UISurface *s);

bool UI_ID_Is_Valid(int id);
bool UI_ID_Register(int id);
int UI_ID_Next(void);

UILayer *UILayer_Create(bool visible, bool enabled);
void UILayer_Destroy(UILayer *l);
UISurface *UILayer_Add_New_Surface(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h);
void UILayer_Update(UILayer *l);
UILayer *UILayer_Find_By_Name(UI *ui, const char *name);
void UILayer_Set_Enabled(UILayer *layer, bool enabled);
void UILayer_Set_Visible(UILayer *layer, bool visible);

UISurface *UISurface_Create(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h);
UIOverlay *UISurface_Add_New_Overlay(UISurface *s, char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, UILayout layout, UIAnchor anchor, UIAlign align, int spacing);
void UISurface_Update(UISurface *s);
void UISurface_Destroy(UISurface *s);
UISurface *UISurface_Load_From_Playscii(const char *filename);
UISurface *UISurface_Find_By_Name(UI *ui, const char *name);
void UISurface_Set_Enabled(UISurface *surface, bool enabled);
void UISurface_Set_Visible(UISurface *surface, bool visible);

UIOverlay *UIOverlay_Create(char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, UILayout layout, UIAnchor anchor, UIAlign align, int spacing);
void UIOverlay_Update(UIOverlay *o);
void UIOverlay_Destroy(UIOverlay *o);
void UIOverlay_Add_New_Element(UIOverlay *o, UIElement *e);
void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *fmt, ...);
UIOverlay *UIOverlay_Find_By_Name(UI *ui, const char *name);
void UIOverlay_Set_Enabled(UIOverlay *overlay, bool enabled);
void UIOverlay_Set_Visible(UIOverlay *overlay, bool visible);

UIElement *UIElement_Create(UIOverlay *o, char *name, int id, int x, int y, int z, int w, int h, int padding, Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled, bool expand, UIAlign align, ElementType type);
void UIElement_Update(UIElement *e);
void UIElement_Destroy(UIElement *e);
UIElement *UIElement_Find_By_Name(UI *ui, const char *e);
void UIElement_Set_Enabled(UIElement *elem, bool enabled);
void UIElement_Set_Visible(UIElement *elem, bool visible);

UIElement_Text *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, UIAlign align, const char *(*cb)(void *ud), void *ud, bool owns_ud);
UIButton *UIButton_Create(UIOverlay *o, const char *name, int id, int x, int y, int z, int w, int h, int padding, Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled, bool expand, UIAlign align, const char *caption, UIButtonAction cb, void *ud);
TextBinding *UIBinding_Text_Create(const void *ptr, const char *fmt,
                                   BindType type);
void UIText_Rebind_To_Data(UIElement_Text *text, const void *ptr, const char *fmt, BindType type);

// Initialize UI action registry
UIActionRegistry *UIAction_Registry_Create(void);
// Destroy UI action registry
void UIAction_Registry_Destroy(UIActionRegistry *registry);
// Register a C function with a string name
bool UIAction_Register(UIActionRegistry *registry, const char *name, void (*c_func)(UIActionContext *ctx));
// Resolve action name to handler
bool UIAction_Resolve(UIActionRegistry *registry, const char *name, UIActionHandler *out_handler);
// Execute action handler
void UIAction_Execute(UIActionHandler *handler, UIActionContext *ctx);
// Resolve all button actions in loaded UI
void UI_Resolve_Button_Actions(UI *ui, UIActionRegistry *registry);
// Update all butons to check inputs and fire events
void UI_Update_Button_Events(UI *ui, Engine *engine);
void UI_Reset_Button_State(void);

const char *UIBinding_Text_Format(void *ud);

const char *pass_through_caption(void *ud);