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
typedef struct UILayer UILayer;
typedef struct UIOverlay UIOverlay;
typedef struct UISurface UISurface;
typedef struct UIElement UIElement;
typedef struct UIText UIText;
typedef struct UIButton UIButton;
typedef struct Cell Cell;
typedef struct cJSON cJSON;

typedef struct PlaysciiAsset {
    int width, height;
    uint32_t *glyphs;
    uint8_t *fg, *bg;
    Color palette[32];
} PlaysciiAsset;

/* ------------------------------------------------------------------------- */
/* UI element types                                                          */
/* ------------------------------------------------------------------------- */

typedef enum {
    UI_ELEM_NONE,
    UI_ELEM_TEXT,
    UI_ELEM_BUTTON,
    UI_ELEM_INPUT
} ElementType;

typedef void (*UIButtonAction)(void *ud);

struct UIElement {
    ElementType type;
    char *name;
    int id;
    bool visible;
    int x, y;
    int width, height;
    void (*update)(struct UIElement *);
};

struct UIText {
    UIElement base;                        /* base element data               */
    const char *(*textCallback)(void *ud); /* callback to provide text        */
    void *ud;                              /* user data for callback          */
    Color_Bzzt fg, bg;                     /* colours for rendering           */
    bool wrap;                             /* enable word wrapping            */
};

struct UIButton {
    UIElement base;
    UIText *label;
    UIButtonAction onClick;
    void *ud;
};

/* ------------------------------------------------------------------------- */
/* Overlay / Surface / Layer                                                 */
/* ------------------------------------------------------------------------- */

struct UIOverlay {
    UISurface *surface;
    UIElement **elements;
    int elements_count, elements_cap;
    int x, y, z;
    bool visible;
    char *name;
    int id;
};

struct UISurface {
    bool visible;
    int x, y, z;
    int w, h;
    Cell *cells;
    int cell_count;
    UIOverlay **overlays;
    int overlays_count, overlays_cap;
    char *name;
    int id;
    Color_Bzzt fg, bg;  /* default colours as defined in YAML          */
    char *bg_img;       /* optional background image path               */
};

struct UILayer {
    bool visible;
    int z;
    UISurface **surfaces;
    int surface_count, surface_cap;
    char *name;
    int id;
};

struct UI {
    UILayer **layers;
    int layer_count, layer_cap;
    bool visible;
    char *name;
    int id;
};

typedef struct UI_Yaml {
    float bzzt_version;
    int palette;     /* placeholder for palette swap implement       */
    bool hot_reload; /* allow hot reloading of the UI on file change */
    char *name;
    int id;
    int layer;
} UI_Yaml;

/* ------------------------------------------------------------------------- */
/* Creation / Destruction                                                    */
/* ------------------------------------------------------------------------- */

cJSON *Playscii_Load(const char *path);
void Playscii_Unload(PlaysciiAsset *asset);

UI *UI_Create();
void UI_Update(UI *ui);
void UI_Destroy(UI *ui);

UILayer *UI_Add_Layer(UI *ui);
void UI_Add_Surface(UI *ui, UISurface *s);
void UI_Set_Visible_Layer(UILayer *, bool show);

UISurface *UISurface_Create(int cell_count);
void UISurface_Add_Overlay(UISurface *s, UIOverlay *o);
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

typedef enum {
    BIND_INT,
    BIND_FLOAT,
    BIND_STR
} BindType;

typedef struct {
    const void *ptr;
    const char *fmt;
    BindType type;
    char buf[32];
} TextBinding;

TextBinding *UIBinding_Text_Create(const void *ptr, const char *fmt,
                                   BindType type);
const char *UIBinding_Text_Format(void *ud);
