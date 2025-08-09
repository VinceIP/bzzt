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
typedef struct Bzzt_Cell Bzzt_Cell;
typedef struct cJSON cJSON;
typedef struct UISurface UISurface;
typedef struct UIOverlay UIOverlay;

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
    BIND_FLOAT,
    BIND_STR
} BindType;

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
    void *parent;
} UIProperties;

// Base struct of UI elements
typedef struct UIElement
{
    ElementType type;
    UIProperties properties;
    Bzzt_Cell *cells;
    int cell_count;
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

typedef struct UIButton
{
    UIElement base;
    UIButtonAction onClick;
    void *ud;
    UIElement_Text *label;
} UIButton;

typedef struct UISurface
{
    UIProperties properties;
    Bzzt_Cell *cells;
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
UISurface *UISurface_Create(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h);

void UISurface_Add_New_Overlay(UISurface *s, char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, UILayout layout, UIAnchor anchor, UIAlign align, int spacing);

void UISurface_Update(UISurface *s);
void UISurface_Destroy(UISurface *s);

UIOverlay *UIOverlay_Create(char *name, int id, int x, int y, int z, int w, int h, int padding, bool visible, bool enabled, UILayout layout, UIAnchor anchor, UIAlign align, int spacing);

void UIOverlay_Update(UIOverlay *o);
void UIOverlay_Destroy(UIOverlay *o);
void UIOverlay_Add_New_Element(UIOverlay *o, UIElement *e);
void UIOverlay_Print(UIOverlay *ov, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *fmt, ...);
UIElement *UIElement_Create(UIOverlay *o, char *name, int id, int x, int y, int z, int w, int h, int padding, Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled, bool expand, ElementType type);
void UIElement_Update(UIElement *e);
void UIElement_Destroy(UIElement *e);
UIElement_Text *UIText_Create(int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, const char *(*cb)(void *ud), void *ud, bool owns_ud);
UIButton *UIButton_Create(UIOverlay *o, const char *name, int id, int x, int y, int z, int w, int h, int padding, Color_Bzzt fg, Color_Bzzt bg, bool visible, bool enabled, bool expand, const char *caption, UIButtonAction cb, void *ud);
bool UISurface_DrawText(UISurface *surface, const char *utf8, int x, int y, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int wrapWidth);
void Renderer_Draw_UI(struct Renderer *, const UI *);
UISurface *UISurface_Load_From_Playscii(const char *filename);
TextBinding *UIBinding_Text_Create(const void *ptr, const char *fmt,
                                   BindType type);
const char *UIBinding_Text_Format(void *ud);