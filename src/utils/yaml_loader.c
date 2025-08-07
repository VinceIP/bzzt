/**
 * @file yaml_loader.c
 * @author Vince Patterson (vinceip532@gmail.com)
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
#include <ctype.h>

extern char *strdup(const char *);

static const char *pass_through(void *ud)
{
    return (const char *)ud;
}

static void measure_text(const char *str, int *w, int *h)
{
    int max_w = 0, cur_w = 0, lines = 1;
    if (!str)
    {
        if (w)
            *w = 0;
        if (h)
            *h = 0;
        return;
    }
    for (int i = 0; str[i];)
    {
        if (str[i] == '\\')
        {
            char next = str[i + 1];
            if (next == 'n')
            {
                if (cur_w > max_w)
                    max_w = cur_w;
                cur_w = 0;
                lines++;
                i += 2;
                continue;
            }
            else if (next == 'f' || next == 'b')
            {
                i += 2;
                while (str[i] && isdigit((unsigned char)str[i]))
                    i++;
                continue;
            }
        }
        else if (str[i] == '\n')
        {
            if (cur_w > max_w)
                max_w = cur_w;
            cur_w = 0;
            lines++;
            i++;
            continue;
        }
        cur_w++;
        i++;
    }
    if (cur_w > max_w)
        max_w = cur_w;
    if (w)
        *w = max_w;
    if (h)
        *h = lines;
}

typedef struct
{
    char *type;
    char *name;
    int id;
    int x, y, z, w, h;
    int padding;
    char *text;
    char *value;
    char *fg;
    char *bg;
    bool *visible;
    bool enabled;
    bool expand;
} YamlElement;

typedef struct
{
    char *name;
    int id;
    char *layout;
    char *anchor;
    int spacing;
    YamlElement *elements;
    unsigned elements_count;
} YamlOverlay;

typedef struct
{
    char *name;
    int id;
    int layer;
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
    YamlSurface *surfaces;
    unsigned surfaces_count;
} YamlUIRoot;

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
    {"transparent", BZ_TRANSPARENT},
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

// --- CYAML schema ---
static const cyaml_schema_field_t element_fields[] = {
    CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER, YamlElement, type, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_OPTIONAL, YamlElement, id),
    CYAML_FIELD_INT("x", CYAML_FLAG_OPTIONAL, YamlElement, x),
    CYAML_FIELD_INT("y", CYAML_FLAG_OPTIONAL, YamlElement, y),
    CYAML_FIELD_INT("z", CYAML_FLAG_OPTIONAL, YamlElement, z),
    CYAML_FIELD_INT("width", CYAML_FLAG_OPTIONAL, YamlElement, w),
    CYAML_FIELD_INT("height", CYAML_FLAG_OPTIONAL, YamlElement, h),
    CYAML_FIELD_INT("padding", CYAML_FLAG_OPTIONAL, YamlElement, padding),
    CYAML_FIELD_STRING_PTR("text", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, text, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("value", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, value, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("fg", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, fg, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("bg", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, bg, 0, CYAML_UNLIMITED),
    CYAML_FIELD_BOOL_PTR("visible", CYAML_FLAG_OPTIONAL, YamlElement, visible),
    CYAML_FIELD_BOOL("enabled", CYAML_FLAG_OPTIONAL, YamlElement, enabled),
    CYAML_FIELD_BOOL("expand", CYAML_FLAG_OPTIONAL, YamlElement, expand),
    CYAML_FIELD_END};

static const cyaml_schema_value_t element_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlElement, element_fields)};

static const cyaml_schema_field_t overlay_fields[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER, YamlOverlay, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_OPTIONAL, YamlOverlay, id),
    CYAML_FIELD_STRING_PTR("layout", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, layout, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("anchor", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, anchor, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("spacing", CYAML_FLAG_OPTIONAL, YamlOverlay, spacing),
    CYAML_FIELD_SEQUENCE("elements", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, elements, &element_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_value_t overlay_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlOverlay, overlay_fields)};

static const cyaml_schema_field_t surface_fields[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_OPTIONAL, YamlSurface, id),
    CYAML_FIELD_INT("layer", CYAML_FLAG_DEFAULT, YamlSurface, layer),
    CYAML_FIELD_INT("width", CYAML_FLAG_DEFAULT, YamlSurface, width),
    CYAML_FIELD_INT("height", CYAML_FLAG_DEFAULT, YamlSurface, height),
    CYAML_FIELD_STRING_PTR("fg", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, fg, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("bg", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, bg, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("x", CYAML_FLAG_DEFAULT, YamlSurface, x),
    CYAML_FIELD_INT("y", CYAML_FLAG_DEFAULT, YamlSurface, y),
    CYAML_FIELD_INT("z", CYAML_FLAG_OPTIONAL, YamlSurface, z),
    CYAML_FIELD_SEQUENCE("overlays", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlSurface, overlays, &overlay_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_value_t surface_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlSurface, surface_fields)};

static const cyaml_schema_field_t root_fields[] = {CYAML_FIELD_SEQUENCE("surfaces", CYAML_FLAG_DEFAULT | CYAML_FLAG_POINTER, YamlUIRoot, surfaces, &surface_schema, 0, CYAML_UNLIMITED), CYAML_FIELD_END};

static const cyaml_schema_value_t root_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, YamlUIRoot, root_fields)};

// --- Loader --------------------------------------------------------

bool UI_Load_From_BUI(UI *ui, const char *path)
{
    Debug_Printf(LOG_UI, "Loading .bui: %s", path);
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
    puts("about to load");
    cyaml_err_t err = cyaml_load_file(path, &config, &root_schema, (cyaml_data_t **)&root, NULL);
    if (err != CYAML_OK)
    {
        Debug_Printf(LOG_UI, "Failed loading YAML. Err: %s", cyaml_strerror(err));
        return false;
    }
    else
        puts("loaded ok");

    bool ok = true;

    // Surfaces
    for (unsigned si = 0; si < root->surfaces_count && ok; ++si)
    {
        YamlSurface *ys = &root->surfaces[si];
        char *surface_name = ys->name ? strdup(ys->name) : NULL;
        int sid;
        if (ys->id > 0)
        {
            sid = ys->id;
            if (!UI_ID_Register(sid))
            {
                Debug_Printf(LOG_UI, "Invalid or duplicate surface id %d", sid);
                ok = false;
                if (surface_name)
                    free(surface_name);
                break;
            }
        }
        else
        {
            sid = UI_ID_Next();
            if (sid < 0)
            {
                Debug_Printf(LOG_UI, "Unable to allocate surface id");
                ok = false;
                if (surface_name)
                    free(surface_name);
                break;
            }
        }
        UISurface *surface = UISurface_Create(NULL, surface_name, sid, true, true,
                                              ys->x, ys->y, ys->z, ys->width, ys->height);
        if (!surface)
        {
            Debug_Printf(LOG_UI, "No surface.");
            ok = false;
            break;
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

        // Overlays
        for (unsigned oi = 0; oi < ys->overlays_count && ok; ++oi)
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
            int oid;
            if (yo->id > 0)
            {
                oid = yo->id;
                if (!UI_ID_Register(oid))
                {
                    Debug_Printf(LOG_UI, "Invalid or duplicate overlay id %d", oid);
                    if (overlay_name)
                        free(overlay_name);
                    ok = false;
                    break;
                }
            }
            else
            {
                oid = UI_ID_Next();
                if (oid < 0)
                {
                    Debug_Printf(LOG_UI, "Unable to allocate overlay id");
                    if (overlay_name)
                        free(overlay_name);
                    ok = false;
                    break;
                }
            }
            UISurface_Add_New_Overlay(surface, overlay_name, oid, 0, 0, 0,
                                      surface->properties.w, surface->properties.h,
                                      0, true, true, layout, anchor, yo->spacing);
            UIOverlay *ov = surface->overlays[surface->overlays_count - 1];
            int y_cursor = 0;

            // Elements
            for (unsigned ei = 0; ei < yo->elements_count && ok; ++ei)
            {
                YamlElement *ye = &yo->elements[ei];
                if (!ye->text && ye->value)
                {
                    ye->text = ye->value;
                    ye->value = NULL;
                }
                if (!ye->type)
                    continue;
                int eid;
                if (ye->id > 0)
                {
                    eid = ye->id;
                    if (!UI_ID_Register(eid))
                    {
                        Debug_Printf(LOG_UI, "Invalid or duplicate element id %d", eid);
                        ok = false;
                        break;
                    }
                }
                else
                {
                    eid = UI_ID_Next();
                    if (eid < 0)
                    {
                        Debug_Printf(LOG_UI, "Unable to allocate element id");
                        ok = false;
                        break;
                    }
                }

                Color_Bzzt elem_fg = color_from_string(ye->fg, COLOR_WHITE);
                Color_Bzzt elem_bg = color_from_string(ye->bg, COLOR_TRANSPARENT);
                bool is_visible = ye->visible ? *ye->visible : true; // visible by default

                if (strcasecmp(ye->type, "Button") == 0)
                {
                    const char *src = ye->text ? ye->text : "";
                    char *caption = strdup(src);
                    UIButton *btn = UIButton_Create(ov, ye->name, ye->id, ye->x, y_cursor + ye->y, ye->z, ye->w, ye->h, ye->padding,
                                                    color_from_string(ye->fg, COLOR_BLACK), color_from_string(ye->bg, COLOR_BLACK),
                                                    is_visible, ye->enabled,
                                                    ye->expand, caption, NULL, NULL);
                    int mw, mh;
                    measure_text(ye->text ? ye->text : "", &mw, &mh);
                    btn->base.properties.w = ye->w > 0 ? ye->w : mw;
                    btn->base.properties.h = ye->h > 0 ? ye->h : mh;
                    UIOverlay_Add_New_Element(ov, (UIElement *)btn);
                    int step = (btn->base.properties.h > 0 ? btn->base.properties.h : 1) + yo->spacing;
                    y_cursor += step;
                }
                else if (strcasecmp(ye->type, "text") == 0)
                {
                    char *dup = ye->text ? strdup(ye->text) : strdup("");
                    UIElement_Text *txt = UIText_Create(ye->x, y_cursor + ye->y, elem_fg, elem_bg, false, pass_through, dup, true);
                    txt->base.properties.name = ye->name ? strdup(ye->name) : NULL;
                    txt->base.properties.id = eid;
                    txt->base.properties.z = ye->z;
                    txt->base.properties.padding = ye->padding;
                    txt->base.properties.expand = ye->expand;
                    txt->base.properties.parent = ov;
                    txt->base.properties.visible = is_visible;
                    txt->base.properties.enabled = ye->enabled;
                    int mw, mh;
                    measure_text(ye->text ? ye->text : "", &mw, &mh);
                    txt->base.properties.w = ye->w > 0 ? ye->w : mw;
                    txt->base.properties.h = ye->h > 0 ? ye->h : mh;
                    UIOverlay_Add_New_Element(ov, (UIElement *)txt);
                    int step = (txt->base.properties.h > 0 ? txt->base.properties.h : 1) + yo->spacing;
                    y_cursor += step;
                }
            }
        }

        if (ok)
        {
            UI_Add_Surface(ui, ys->layer, surface);
        }
        else
        {
            UISurface_Destroy(surface);
        }
    }
    cyaml_free(&config, &root_schema, root, 0);
    return ok;
}