#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "color.h"

struct Color;
typedef struct UILayer UILayer;
typedef struct UIOverlay UIOverlay;
typedef struct UISurface UISurface;
typedef struct Cell Cell;
typedef struct cJSON cJSON;

typedef struct PlaysciiAsset
{
    int width, height;
    uint32_t *glyphs;
    uint8_t *fg, *bg;
    Color palette[32];
} PlaysciiAsset;

/**
 * @brief An encapsulated UI which holds multiple UI layers.
 *
 */
typedef struct UI
{
    UILayer **layers;
    int layer_count, layer_cap;
    bool visible;
} UI;

cJSON *Playscii_Load(const char *path);
void Playscii_Unload(PlaysciiAsset *asset);

/**
 * @brief Instantiate a new UI.
 *
 * @return UI*
 */
UI *UI_Create(void);
void UI_Update(UI *ui);
void UI_Destroy(UI *ui);

/**
 * @brief Push a new UILayer to UI.
 *
 * @param ui
 * @return UILayer*
 */
UILayer *UI_Add_Layer(UI *ui);
void UI_Add_Surface(UI *ui, UISurface *s);

void UI_Set_Visible_Layer(UILayer *, bool show);

/**
 * @brief Print text to screen-relative coordinates
 *
 * @param fg fb color
 * @param bg bg color
 * @param wrap Enable text wrapping at screen edge
 * @param x
 * @param y
 * @param fmt String with/without format specifiers
 * @param ... Formatting args
 */
void UI_Print_Screen(UI *ui, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int x, int y, char *fmt, ...);

UISurface *UISurface_Load_From_Playscii(const char *filename);
