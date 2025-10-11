/**
 * @file bui_loader.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief A robust BUI loader with correct default value handling.
 * @version 1.0
 * @date 2025-08-15
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "bui_loader.h"
#include "ui.h"
#include "color.h"
#include "bzzt.h"
#include "debugger.h"
#include <cyaml/cyaml.h>
#include <strings.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
            else if (next == 'f' || next == 'b' || next == 'c')
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
    char *on_down;
    char *on_press;
    char *on_release;
} YamlButtonEvents;

typedef struct
{
    char *type;   // keboard, mouse, gamepad
    char *key;    // key_q, key_enter, etc
    char *button; // mouse_left, gamepad_button_a, etc
} YamlInputBinding;

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
    char *align;
    bool *visible;
    bool *enabled;
    bool expand;

    YamlInputBinding *input_bindings;
    unsigned input_bindings_count;

    YamlButtonEvents *events;
} YamlElement;

typedef struct
{
    char *name;
    int id;
    bool *enabled;
    bool *visible;
    int x, y, z, w, h;
    int padding;
    char *layout;
    char *anchor;
    char *align;
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

/**
 * @brief REFINED: Correctly handles COLOR_TRANSPARENT, which has an enum value that
 * would be incorrectly processed by bzzt_get_color's bitmask.
 */
static Color_Bzzt color_from_string(const char *s, Color_Bzzt default_color)
{
    if (!s)
        return default_color;
    if (strcasecmp(s, "transparent") == 0)
        return COLOR_TRANSPARENT;
    for (size_t i = 0; i < COLOR_MAP_COUNT; ++i)
    {
        if (strcasecmp(s, color_map[i].str) == 0)
        {
            if (color_map[i].val == BZ_TRANSPARENT)
                return COLOR_TRANSPARENT;
            return bzzt_get_color(color_map[i].val);
        }
    }
    return default_color;
}

/**
 * @brief REFINED: Added "center" as a user-friendly alias for "middle_center".
 */
static UIAnchor anchor_from_string(const char *s)
{
    if (!s)
        return ANCHOR_TOP_LEFT;
    if (strcasecmp(s, "top-left") == 0)
        return ANCHOR_TOP_LEFT;
    if (strcasecmp(s, "top-center") == 0)
        return ANCHOR_TOP_CENTER;
    if (strcasecmp(s, "top-right") == 0)
        return ANCHOR_TOP_RIGHT;
    if (strcasecmp(s, "middle-left") == 0)
        return ANCHOR_MIDDLE_LEFT;
    if (strcasecmp(s, "center") == 0 || strcasecmp(s, "middle-center") == 0)
        return ANCHOR_MIDDLE_CENTER;
    if (strcasecmp(s, "middle-right") == 0)
        return ANCHOR_MIDDLE_RIGHT;
    if (strcasecmp(s, "bottom-left") == 0)
        return ANCHOR_BOTTOM_LEFT;
    if (strcasecmp(s, "bottom-center") == 0)
        return ANCHOR_BOTTOM_CENTER;
    if (strcasecmp(s, "bottom-right") == 0)
        return ANCHOR_BOTTOM_RIGHT;
    return ANCHOR_TOP_LEFT; // Default
}

static UIAlign align_from_string(const char *s)
{
    if (!s)
        return ALIGN_LEFT;
    if (strcasecmp(s, "center") == 0)
        return ALIGN_CENTER;
    if (strcasecmp(s, "right") == 0)
        return ALIGN_RIGHT;
    return ALIGN_LEFT; // Default
}

// --- CYAML schema ---

static const cyaml_schema_field_t input_binding_fields[] = {
    CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlInputBinding, type, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("key", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlInputBinding, key, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("button", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlInputBinding, button, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_value_t input_binding_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlInputBinding, input_binding_fields)};

static const cyaml_schema_field_t button_events_fields[] = {
    CYAML_FIELD_STRING_PTR("on_down", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlButtonEvents, on_down, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("on_press", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlButtonEvents, on_press, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("on_release", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlButtonEvents, on_release, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END};

static const cyaml_schema_value_t button_events_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlButtonEvents, button_events_fields)};

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
    CYAML_FIELD_STRING_PTR("align", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, align, 0, CYAML_UNLIMITED),
    CYAML_FIELD_BOOL_PTR("visible", CYAML_FLAG_OPTIONAL, YamlElement, visible),
    CYAML_FIELD_BOOL_PTR("enabled", CYAML_FLAG_OPTIONAL, YamlElement, enabled),
    CYAML_FIELD_BOOL("expand", CYAML_FLAG_OPTIONAL, YamlElement, expand),

    CYAML_FIELD_SEQUENCE("input_bindings", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlElement, input_bindings,
                         &input_binding_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_MAPPING_PTR("events", CYAML_FLAG_OPTIONAL, YamlElement, events, button_events_fields),

    CYAML_FIELD_END};

static const cyaml_schema_value_t element_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT, YamlElement, element_fields)};

static const cyaml_schema_field_t overlay_fields[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER, YamlOverlay, name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT("id", CYAML_FLAG_OPTIONAL, YamlOverlay, id),
    CYAML_FIELD_BOOL_PTR("visible", CYAML_FLAG_OPTIONAL, YamlOverlay, visible),
    CYAML_FIELD_BOOL_PTR("enabled", CYAML_FLAG_OPTIONAL, YamlOverlay, enabled),
    CYAML_FIELD_INT("x", CYAML_FLAG_OPTIONAL, YamlOverlay, x),
    CYAML_FIELD_INT("y", CYAML_FLAG_OPTIONAL, YamlOverlay, y),
    CYAML_FIELD_INT("z", CYAML_FLAG_OPTIONAL, YamlOverlay, z),
    CYAML_FIELD_INT("w", CYAML_FLAG_OPTIONAL, YamlOverlay, w),
    CYAML_FIELD_INT("h", CYAML_FLAG_OPTIONAL, YamlOverlay, h),
    CYAML_FIELD_INT("padding", CYAML_FLAG_OPTIONAL, YamlOverlay, padding),
    CYAML_FIELD_STRING_PTR("layout", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, layout, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("anchor", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, anchor, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("align", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER, YamlOverlay, align, 0, CYAML_UNLIMITED),
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

static int parse_raylib_key(const char *key_str)
{
    if (!key_str)
        return 0;

    // handle common keys
    // tbd - make case insensitive
    if (strcmp(key_str, "KEY_Q") == 0)
        return KEY_Q;
    if (strcmp(key_str, "KEY_P") == 0)
        return KEY_P;
    if (strcmp(key_str, "KEY_E") == 0)
        return KEY_E;
    if (strcmp(key_str, "KEY_ENTER") == 0)
        return KEY_ENTER;
    if (strcmp(key_str, "KEY_SPACE") == 0)
        return KEY_SPACE;
    if (strcmp(key_str, "KEY_ESCAPE") == 0)
        return KEY_ESCAPE;
    if (strcmp(key_str, "KEY_Y") == 0)
        return KEY_Y;
    if (strcmp(key_str, "KEY_N") == 0)
        return KEY_N;

    Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Unknown key: %s", key_str);
    return 0;
}

static int parse_raylib_mouse_button(const char *button_str)
{
    if (!button_str)
        return 0;

    if (strcmp(button_str, "MOUSE_LEFT") == 0)
        return MOUSE_LEFT_BUTTON;
    if (strcmp(button_str, "MOUSE_RIGHT") == 0)
        return MOUSE_RIGHT_BUTTON;
    if (strcmp(button_str, "MOUSE_MIDDLE") == 0)
        return MOUSE_MIDDLE_BUTTON;

    Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Unknown mouse button: %s", button_str);
    return 0;
}

static int parse_raylib_gamepad_button(const char *button_str)
{
    if (!button_str)
        return 0;

    if (strcmp(button_str, "GAMEPAD_BUTTON_A") == 0)
        return GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
    if (strcmp(button_str, "GAMEPAD_BUTTON_B") == 0)
        return GAMEPAD_BUTTON_RIGHT_FACE_RIGHT;
    if (strcmp(button_str, "GAMEPAD_BUTTON_X") == 0)
        return GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
    if (strcmp(button_str, "GAMEPAD_BUTTON_Y") == 0)
        return GAMEPAD_BUTTON_RIGHT_FACE_UP;

    Debug_Log(LOG_LEVEL_WARN, LOG_UI, "Unknown gamepad button: %s", button_str);
    return 0;
}

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

    cyaml_err_t err = cyaml_load_file(path, &config, &root_schema, (cyaml_data_t **)&root, NULL);
    if (err != CYAML_OK)
    {
        Debug_Printf(LOG_UI, "Failed loading YAML. Err: %s", cyaml_strerror(err));
        return false;
    }

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
            UILayout layout = LAYOUT_NONE;
            if (yo->layout && strcasecmp(yo->layout, "vbox") == 0)
                layout = LAYOUT_VBOX;

            UIAnchor anchor = anchor_from_string(yo->anchor);
            UIAlign align = align_from_string(yo->align);
            char *overlay_name = yo->name ? strdup(yo->name) : NULL;

            // Check if the optional 'visible' field was present. If not (pointer is NULL), default to true.
            bool is_overlay_visible = yo->visible ? *yo->visible : true;
            // Check if the optional 'enabled' field was present. If not (pointer is NULL), default to true.
            bool is_overlay_enabled = yo->enabled ? *yo->enabled : true;

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

            UISurface_Add_New_Overlay(surface, overlay_name, oid,
                                      yo->x, yo->y, yo->z,
                                      yo->w, yo->h, yo->padding,
                                      is_overlay_visible, is_overlay_enabled,
                                      layout, anchor, align, yo->spacing);
            UIOverlay *ov = surface->overlays[surface->overlays_count - 1];
            // int y_cursor = 0;

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

                // Check for optional 'visible' and 'enabled' fields for elements. Default to true.
                bool is_elem_visible = ye->visible ? *ye->visible : true;
                bool is_elem_enabled = ye->enabled ? *ye->enabled : true;

                // handle buttons
                if (strcasecmp(ye->type, "Button") == 0)
                {
                    const char *src = ye->text ? ye->text : "";
                    char *caption = strdup(src);
                    UIAlign align = align_from_string(ye->align);
                    UIButton *btn = UIButton_Create(ov, ye->name, eid, ye->x, ye->y, ye->z, ye->w, ye->h, ye->padding,
                                                    elem_fg, elem_bg,
                                                    is_elem_visible, is_elem_enabled,
                                                    ye->expand, align, caption,
                                                    NULL, NULL);

                    int mw, mh;
                    measure_text(caption ? caption : "", &mw, &mh);
                    btn->base.properties.w = ye->w > 0 ? ye->w : mw;
                    btn->base.properties.h = ye->h > 0 ? ye->h : mh;

                    if (ye->input_bindings && ye->input_bindings_count > 0)
                    {
                        btn->input_bindings.count = 0;
                        for (int i = 0; i < ye->input_bindings_count && i < 8; ++i)
                        {
                            YamlInputBinding *yb = &ye->input_bindings[i];
                            UIInputBinding *ib =
                                &btn->input_bindings.bindings[btn->input_bindings.count];

                            if (strcmp(yb->type, "keyboard") == 0)
                            {
                                ib->type = KEYBOARD;
                                ib->code = parse_raylib_key(yb->key);
                                if (ib->code != 0)
                                    btn->input_bindings.count++;
                            }
                            else if (strcmp(yb->type, "mouse") == 0)
                            {
                                ib->type = MOUSE;
                                ib->code = parse_raylib_mouse_button(yb->button);
                                if (ib->code != 0)
                                    btn->input_bindings.count++;
                            }
                            else if (strcmp(yb->type, "gamepad") == 0)
                            {
                                ib->type = GAMEPAD;
                                ib->code = parse_raylib_gamepad_button(yb->button);
                                if (ib->code != 0)
                                    btn->input_bindings.count++;
                            }
                        }
                    }

                    if (ye->events)
                    {
                        if (ye->events->on_down)
                            btn->events.on_down_action = strdup(ye->events->on_down);
                        if (ye->events->on_press)
                            btn->events.on_press_action = strdup(ye->events->on_press);
                        if (ye->events->on_release)
                            btn->events.on_release_action = strdup(ye->events->on_release);
                    }

                    UIOverlay_Add_New_Element(ov, (UIElement *)btn);
                    int step = (btn->base.properties.h > 0 ? btn->base.properties.h : 1) + yo->spacing;
                    // y_cursor += step;
                }

                else if (strcasecmp(ye->type, "text") == 0)
                {
                    char *dup = ye->text ? strdup(ye->text) : strdup("");
                    UIAlign align = align_from_string(ye->align);

                    UIElement_Text *txt = UIText_Create(ye->x, ye->y, elem_fg, elem_bg, false, align, pass_through, dup, true);
                    txt->base.properties.name = ye->name ? strdup(ye->name) : NULL;
                    txt->base.properties.id = eid;
                    txt->base.properties.z = ye->z;
                    txt->base.properties.padding = ye->padding;
                    txt->base.properties.expand = ye->expand;
                    txt->base.properties.parent = ov;
                    txt->base.properties.visible = is_elem_visible;
                    txt->base.properties.enabled = is_elem_enabled;
                    int mw, mh;
                    measure_text(ye->text ? ye->text : "", &mw, &mh);
                    txt->base.properties.w = ye->w > 0 ? ye->w : mw;
                    txt->base.properties.h = ye->h > 0 ? ye->h : mh;
                    UIOverlay_Add_New_Element(ov, (UIElement *)txt);
                    int step = (txt->base.properties.h > 0 ? txt->base.properties.h : 1) + yo->spacing;
                    // y_cursor += step;
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