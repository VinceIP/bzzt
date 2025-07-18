#pragma once
/*
    Central header for all UI components.  This file exposes every public
    structure and function used by the bzzt UI system so that source files only
    need to include a single header.  Former individual headers now simply
    include this one.
*/

#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "color.h"

struct Color;
typedef struct Renderer Renderer;
typedef struct Cell Cell;
typedef struct cJSON cJSON;
typedef struct UISurface UISurface;
typedef struct UIOverlay UIOverlay;

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
    BIND_FLOAT,
    BIND_STR
} BindType;

typedef struct
{
    const void *ptr;
    const char *fmt;
    BindType type;
    char buf[32];
} TextBinding;

typedef enum
{
    UI_ELEM_NONE,
    UI_ELEM_TEXT,
    UI_ELEM_BUTTON,
    UI_ELEM_INPUT
} ElementType;

typedef enum
{
    LAYOUT_NONE,
    LAYOUT_VBOX,
    LAYOUT_HBOX
} OverlayLayout;

typedef enum
{
    ANCHOR_NONE,
    ANCHOR_LEFT,
    ANCHOR_CENTER,
    ANCHOR_RIGHT,
    ANCHOR_TOP,
    ANCHOR_BOTTOM
} OverlayAnchor;

typedef void (*UIButtonAction)(void *ud);

// Properties shared by most UI components
typedef struct UIProperties
{
    char *name;
    int id;
    int x, y, z, w, h;
    int padding;
    bool visible; // Only drawn if visible
    bool enabled; // If disabled, all drawing and callbacks stop until enabled again
    void *parent; // Reference to the parent struct
} UIProperties;

// Base struct of UI elements
typedef struct UIElement
{
    ElementType type;
    UIProperties properties;
    void (*update)(struct UIElement *);
} UIElement;

// A UI text field
typedef struct UIText
{
    UIElement base;
    const char *(*textCallback)(void *ud);
    void *ud;
    Color_Bzzt fg, bg; // Color of the text to be printed to screen
    bool wrap;         // Does the text wrap and start a newline if out of bounds
} UIText;

typedef struct UIButton
{
    UIText uiText; // A button is essentially just a UIText, except it will hook into specific event things below, probably
} UIButton;

/* Overlay / Surface / Layer*/

typedef struct UISurface
{
    UIProperties properties;
    Cell *cells; // Default cells this surface holds - could be none, could be a background
    int cell_count;
    UIOverlay **overlays;
    int overlays_count, overlays_cap;
} UISurface;

typedef struct UIOverlay
{
    UIProperties properties;
    UIElement **elements;
    int elements_count, elements_cap;
    OverlayLayout layout;
    OverlayAnchor anchor;
    int spacing;
} UIOverlay;

typedef struct UILayer
{
    char *name;
    int id;
    bool visible, enabled;
    int index; // The index of this layer in the UI's stack
    UISurface **surfaces;
    int surface_count, surface_cap;
} UILayer;

typedef struct UI
{
    bool visible, enabled;
    UILayer **layers;
    int layer_count, layer_cap;
} UI;

cJSON *Playscii_Load(const char *path);
void Playscii_Unload(PlaysciiAsset *asset);

/**
 * @brief Instantiate a new UI.
 *
 * @param visible Is visible on creation
 * @param enabled Is enabled on creation
 * @return UI*
 */
UI *UI_Create(bool visible, bool enabled);

/**
 * @brief Run update callbacks on UI.
 *
 * @param ui
 */
void UI_Update(UI *ui);

/**
 * @brief Free UI and all its components from memory.
 *
 * @param ui
 */
void UI_Destroy(UI *ui);

/**
 * @brief Creates a new UILayer and pushes it to the UI.
 *
 * @param ui
 * @param visible Is visible on creation
 * @param enabled Is enabled on creation
 * @return UILayer*
 */
UILayer *UI_Add_New_UILayer(UI *ui, bool visible, bool enabled);

/**
 * @brief Creates a new UILayer.
 *
 * @param visible Is visible on creation
 * @param enabled Is enabled on creation
 * @return UILayer*
 */
UILayer *UILayer_Create(bool visible, bool enabled);

/**
 * @brief Destroys a UILayer and frees it from memory.
 *
 * @param l
 */
void UILayer_Destroy(UILayer *l);

/**
 * @brief Creates a new UISurface and pushes it to the target UILayer.
 * 
 * @param l Target UILayer
 * @param name name of new surface
 * @param id id of new surface
 * @param visible surface is visible on creation
 * @param enabled surface is enabled on creation
 * @param x coordinates in screen-cell units
 * @param y coordinates in screen-cell units
 * @param z z layer of new surface, relative to other surfaces on this layer
 * @param w width of new layer in cell units
 * @param h height of new layer in cell units
 * @return UISurface* 
 */
UISurface *UILayer_Add_New_Surface(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h);

/**
 * @brief Run update callbacks on this UILayer.
 * 
 * @param l 
 */
void UILayer_Update(UILayer *l);

/**
 * @brief Create a new UISurface.
 * 
 * @param l target layer the new surface is stored on
 * @param name name of new surface
 * @param id id of new surface
 * @param visible is visible on creation
 * @param enabled is enabled on creation
 * @param x coordinates in screen-cell units
 * @param y coordinates in screen-cell units
 * @param z z layer of new surface, relative to other surfaces on this layer
 * @param w width of new layer in cell units
 * @param h height of new layer in cell units
 * @return UISurface* 
 */
UISurface *UISurface_Create(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h);

void UISurface_Add_New_Overlay(UISurface *s, UIOverlay *o);
void UISurface_Update(UISurface *s);
void UISurface_Destroy(UISurface *s);

UIOverlay *UIOverlay_Create();
void UIOverlay_Update(UIOverlay *o);
void UIOverlay_Destroy(UIOverlay *o);
void UIOverlay_Add_Element(UIOverlay *o, UIElement *e);
void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap,
                     const char *fmt, ...);

UIElement *UIElement_Create();
void UIElement_Update(UIElement *e);
void UIElement_Destroy(UIElement *e);
UIText *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg,
                      const char *(*cb)(void *), void *ud);
UIText *UIText_Create_Bound(int x, int y, Color_Bzzt fg, Color_Bzzt bg,
                            const void *ptr, const void *fmt, BindType type);
UIButton *UIButton_Create(int x, int y, const char *caption,
                          UIButtonAction cb, void *ud);

bool UISurface_DrawText(UISurface *surface, const char *utf8, int x, int y,
                        Color_Bzzt fg, Color_Bzzt bg, bool wrap, int wrapWidth);

void Renderer_Draw_UI(struct Renderer *, const UI *);

void UI_Print_Screen(UI *ui, UISurface *s, Color_Bzzt fg, Color_Bzzt bg,
                     bool wrap, int x, int y, char *fmt, ...);

UISurface *UISurface_Load_From_Playscii(const char *filename);

TextBinding *UIBinding_Text_Create(const void *ptr, const char *fmt,
                                   BindType type);
const char *UIBinding_Text_Format(void *ud);
