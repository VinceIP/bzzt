#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"
#include "raylib.h"
#include "object.h"

typedef struct
{
    int width, height;
    uint32_t *glyphs;
    uint8_t *fg, *bg;
    Color palette[32];
} PlaysciiAsset;

/**
 * @brief A surface on which to draw Cells.
 * 
 */
typedef struct
{
    Cell *cells;
    int width, height;
} UISurface;

/**
 * @brief An encapsulated UI which holds all UISurfaces.
 * 
 */
typedef struct
{
    int count, cap;
    UISurface **surfaces;
} UI;

/**
 * @brief A surface to be overlayed on a UISurface and toggled when needed.
 * For printing text to a UI element, etc
 * 
 */
typedef struct{
    UISurface *surface;
    bool visible;
} UIOverlay;

cJSON *Playscii_Load(const char *path);
void Playscii_Unload(PlaysciiAsset *asset);
/**
 * @brief Instantiate a new UI.
 * 
 * @return UI* 
 */
UI *UI_Create(void);

/**
 * @brief Push a new UISurface to the UI.
 * 
 * @param ui Main engine's UI
 * @param s  A UISurface to push.
 */
void UI_Add_Surface(UI *ui, UISurface *s);
/**
 * @brief Print text to a UISurface.
 * 
 * @param s 
 * @param str 
 * @param pos 
 */
void UI_Draw_Text_To_Surface(UISurface *s, const char* str, Vector2 pos);
void UI_Destroy(UI *ui);
UISurface *UISurface_Create(int cell_count);
void UISurface_Destroy(UISurface *surface);
UISurface *UISurface_Load_From_Playscii(const char *filename);