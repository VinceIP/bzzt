/**
 * @file yaml_loader.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "yaml_loader.h"
#include "ui.h"
#include "color.h"
#include "bzzt.h"
#include "debugger.h"
#include <cyaml/cyaml.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// --- YAML <-> struct definitions ---------------------------------

typedef struct
{
    char *type;
    char *name;
    int id;
    int x, y, z, w, h;
    int padding;
    char *text;
} YamlButton;

typedef struct
{
    char *name;
    int id;
    char *layout;
    char *anchor;
    int spacing;
    YamlButton *elements;
    unsigned elements_count;
} YamlOverlay;

typedef struct
{
    int width;
    int height;
    char *fg;
    char *bg;
    int x;
    int y;
    int z;
    YamlOverlay *overlays;
    unsigned overlays_count;
} YamlSurface;

typedef struct
{
    char *name;
    int id;
    int layer;
    YamlSurface surface;
} YamlUIRoot;

// --- Color helpers -------------------------------------------------

typedef struct
{
    const char *str;
    BzztColor val;
} ColorMap;
static const ColorMap color_map[] = {
    {"black", BZ_BLACK},
    {"blue", BZ_BLUE},
    {"green", BZ_GREEN},
    {"cyan", BZ_CYAN},
    {"red", BZ_RED},
    {"magenta", BZ_MAGENTA},
    {"brown", BZ_BROWN},
    {"light_gray", BZ_LIGHT_GRAY},
    {"dark_gray", BZ_DARK_GRAY},
    {"light_blue", BZ_LIGHT_BLUE},
    {"light_green", BZ_LIGHT_GREEN},
    {"light_cyan", BZ_LIGHT_CYAN},
    {"light_red", BZ_LIGHT_RED},
    {"light_magenta", BZ_LIGHT_MAGENTA},
    {"yellow", BZ_YELLOW},
    {"white", BZ_WHITE},
};
#define COLOR_MAP_COUNT (sizeof(color_map) / sizeof(color_map[0]))

static Color_Bzzt color_from_string(const char *s, Color_Bzzt default_color)
{
    if (!s)
        return default_color;
    for (size_t i = 0; i < COLOR_MAP_COUNT; ++i)
    {
        if (strcasecmp(s, color_map[i].str) == 0)
        {
            return bzzt_get_color(color_map[i].val);
        }
    }
    return default_color;
}
// --- CYAML schema --------------------------------------------------

static const cyaml_schema_field_t button_fields[] = {
    CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER, YamlButton, type, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlButton, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_OPTIONAL, YamlButton, id),
    CYAML_FIELD_INT("x", CYAML_FLAG_OPTIONAL, YamlButton, x),
    CYAML_FIELD_INT("y", CYAML_FLAG_OPTIONAL, YamlButton, y),
    CYAML_FIELD_INT("z", CYAML_FLAG_OPTIONAL, YamlButton, z),
    CYAML_FIELD_INT("w", CYAML_FLAG_OPTIONAL, YamlButton, w),
    CYAML_FIELD_INT("h", CYAML_FLAG_OPTIONAL, YamlButton, h),
    CYAML_FIELD_INT("padding", CYAML_FLAG_OPTIONAL, YamlButton, padding),
    CYAML_FIELD_STRING_PTR("text", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlButton, text, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_value_t button_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlButton, button_fields)};

static const cyaml_schema_field_t overlay_fields[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER, YamlOverlay, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_DEFAULT, YamlOverlay, id),
    CYAML_FIELD_STRING_PTR("layout", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, layout, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("anchor", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, anchor, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("spacing", CYAML_FLAG_OPTIONAL, YamlOverlay, spacing),
    CYAML_FIELD_SEQUENCE("elements", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, elements, &button_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_value_t overlay_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlOverlay, overlay_fields)};

static const cyaml_schema_field_t surface_fields[] = {
    CYAML_FIELD_INT("width", CYAML_FLAG_DEFAULT, YamlSurface, width),
    CYAML_FIELD_INT("height", CYAML_FLAG_DEFAULT, YamlSurface, height),
    CYAML_FIELD_STRING_PTR("fg", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, fg, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("bg", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, bg, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("x", CYAML_FLAG_DEFAULT, YamlSurface, x),
    CYAML_FIELD_INT("y", CYAML_FLAG_DEFAULT, YamlSurface, y),
    CYAML_FIELD_INT("z", CYAML_FLAG_OPTIONAL, YamlSurface, z),
    CYAML_FIELD_SEQUENCE("overlays", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, overlays, &overlay_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_field_t root_fields[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER, YamlUIRoot, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_DEFAULT, YamlUIRoot, id),
    CYAML_FIELD_INT("layer", CYAML_FLAG_DEFAULT, YamlUIRoot, layer),
    CYAML_FIELD_MAPPING("surface", CYAML_FLAG_DEFAULT, YamlUIRoot, surface, surface_fields),
    CYAML_FIELD_END};

static const cyaml_schema_value_t root_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, YamlUIRoot, root_fields)};

// --- Loader --------------------------------------------------------

bool UI_Load_From_BUI(UI *ui, const char *path)
{
    if (!ui || !path)
    {
        Debug_Printf(LOG_UI, "Failed loading UI.");
        return false;
    }

    static const cyaml_config_t config = {
        .log_level = CYAML_LOG_WARNING,
        .mem_fn = cyaml_mem,
        .mem_ctx = NULL,
        .flags = CYAML_CFG_DEFAULT};

    YamlUIRoot *root = NULL;
    cyaml_err_t err = cyaml_load_file(path, &config, &root_schema, (cyaml_data_t **)&root, NULL);
    if (err != CYAML_OK)
    {
        Debug_Printf(LOG_UI, "Failed loading YAML. Err: %s", cyaml_strerror(err));
        return false;
    }

    YamlSurface *ys = &root->surface;

    char *surface_name = root->name ? strdup(root->name) : NULL;
    UISurface *surface = UISurface_Create(NULL, surface_name, root->id, true, true,
                                          ys->x, ys->y, ys->z, ys->width, ys->height);
    if (!surface)
    {
        Debug_Printf(LOG_UI, "No surface.");
        cyaml_free(&config, &root_schema, root, 0);
        return false;
    }

    Color_Bzzt fg = color_from_string(ys->fg, COLOR_WHITE);
    Color_Bzzt bg = color_from_string(ys->bg, COLOR_TRANSPARENT);
    for (int i = 0; i < surface->cell_count; ++i)
    {
        surface->cells[i].visible = true;
        surface->cells[i].glyph = 255;
        surface->cells[i].fg = fg;
        surface->cells[i].bg = bg;
    }

    UI_Add_Surface(ui, root->layer, surface);

    // Overlays
    for (unsigned oi = 0; oi < ys->overlays_count; ++oi)
    {
        YamlOverlay *yo = &ys->overlays[oi];
        OverlayLayout layout = LAYOUT_NONE;
        if (yo->layout && strcasecmp(yo->layout, "vbox") == 0)
            layout = LAYOUT_VBOX;
        OverlayAnchor anchor = ANCHOR_NONE;
        if (yo->anchor)
        {
            if (strcasecmp(yo->anchor, "center") == 0)
                anchor = ANCHOR_CENTER;
            else if (strcasecmp(yo->anchor, "left") == 0)
                anchor = ANCHOR_LEFT;
            else if (strcasecmp(yo->anchor, "right") == 0)
                anchor = ANCHOR_RIGHT;
        }
        char *overlay_name = yo->name ? strdup(yo->name) : NULL;
        UISurface_Add_New_Overlay(surface, overlay_name, yo->id, 0, 0, 0,
                                  surface->properties.w, surface->properties.h,
                                  0, true, true, layout, anchor, yo->spacing);
        UIOverlay *ov = surface->overlays[surface->overlays_count - 1];
        int y_cursor = 0;
        // Elements
        for (unsigned ei = 0; ei < yo->elements_count; ++ei)
        {
            YamlButton *yb = &yo->elements[ei];
            if (yb->type && strcasecmp(yb->type, "Button") == 0)
            {
                UIButton *btn = UIButton_Create(0, y_cursor, yb->text ? yb->text : "", NULL, NULL);
                btn->base.properties.name = yb->name ? strdup(yb->name) : NULL;
                btn->base.properties.id = yb->id;
                if (yb->w > 0)
                    btn->base.properties.w = yb->w;
                if (yb->h > 0)
                    btn->base.properties.h = yb->h;
                UIOverlay_Add_New_Element(ov, (UIElement *)btn);
                int step = (btn->base.properties.h > 0 ? btn->base.properties.h : 1) + yo->spacing;
                y_cursor += step;
            }
        }
    }

    cyaml_free(&config, &root_schema, root, 0);
    return true;
}
