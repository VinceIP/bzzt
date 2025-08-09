/**
 * @file ui.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-08-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "ui.h"
#include "cJSON.h"
#include "bzzt.h"
#include "color.h"
#include "debugger.h"
#include "engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define CHECK_FAIL(cond, label, msg)                               \
    do                                                             \
    {                                                              \
        if (!(cond))                                               \
        {                                                          \
            Debug_Printf(LOG_UI, "UISurface load error: %s", msg); \
            goto label;                                            \
        }                                                          \
    } while (0)

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror(path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *buf = malloc(len + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

static int grow_layers(UI *ui)
{
    int new_cap = ui->layer_cap == 0 ? 4 : ui->layer_cap * 2;
    UILayer **tmp = realloc(ui->layers, sizeof(UILayer *) * new_cap);
    if (!tmp)
        return -1;
    ui->layers = tmp;
    ui->layer_cap = new_cap;
    return 0;
}

UI *UI_Create(bool visible, bool enabled)
{
    Debug_Printf(LOG_UI, "Creating a new base UI.");
    UI *ui = calloc(1, sizeof(UI));
    if (!ui)
    {
        Debug_Printf(LOG_UI, "Failed to allocate base UI.");
        goto cleanup;
    }

    ui->visible = visible;
    ui->enabled = enabled;
    ui->layer_count = 0;
    ui->layer_cap = 4;

    ui->layers = malloc(sizeof(UILayer *) * ui->layer_cap);
    if (!ui->layers)
    {
        Debug_Printf(LOG_UI, "Failed to allocate UI layers stack.");
        goto cleanup;
    }

    puts("returning base ui");
    return ui;

cleanup:
    free(ui);
    return NULL;
}

void UI_Destroy(UI *ui)
{
    if (!ui)
        return;
    int layer_count = ui->layer_count;
    for (int i = 0; i < layer_count; ++i)
    {
        UILayer *l = ui->layers[i];
        UILayer_Destroy(l);
    }
    free(ui->layers);
    free(ui);
}

void UI_Add_Surface(UI *ui, int targetIndex, UISurface *s)
{
    if (!ui || !s)
    {
        Debug_Printf(LOG_UI, "One or more params was null on UI_Add_Surface.");
        return;
    }

    int cell_count = s->cell_count;
    Debug_Printf(LOG_UI, "Adding a surface with %d cells to a UI layer.", cell_count);

    if (ui->layer_count <= targetIndex)
    {
        if (ui->layer_count == 0)
        {
            UI_Add_New_Layer(ui, true, true); // Add initial layer if none exists
        }
        Debug_Printf(LOG_UI, "UI_Add_Surface targeted layer %d, but that layer doesn't exist. Adding it now.", targetIndex);
        for (int i = ui->layer_count; i <= targetIndex; ++i)
        {
            UI_Add_New_Layer(ui, true, true);
        }
    }

    UILayer *layer = ui->layers[targetIndex];

    if (!layer)
        Debug_Printf(LOG_UI, "UI_Add_Surface was unable to target layer at index: %d", targetIndex);

    if (layer->surface_count >= layer->surface_cap)
    {
        int new_cap = layer->surface_cap == 0 ? 4 : layer->surface_cap * 2;
        UISurface **tmp = realloc(layer->surfaces, sizeof(UISurface *) * new_cap);
        if (!tmp)
            return;
        layer->surfaces = tmp;
        layer->surface_cap = new_cap;
    }
    layer->surfaces[layer->surface_count++] = s;
}

UILayer *UI_Add_New_Layer(UI *ui, bool visible, bool enabled)
{
    Debug_Printf(LOG_UI, "Adding a layer to UI.");
    if (!ui)
        return NULL;

    if (ui->layer_count >= ui->layer_cap && grow_layers(ui) != 0)
        return NULL;

    UILayer *layer = UILayer_Create(visible, enabled);
    if (!layer)
        return NULL;

    ui->layers[ui->layer_count++] = layer;
    layer->index = ui->layer_count - 1; // Assign the new layer's index to the current ui layer count
    return layer;
}

void UI_Update(UI *ui)
{
    if (!ui || !ui->visible)
        return;

    for (int i = 0; i < ui->layer_count; ++i)
    {
        UILayer *l = ui->layers[i];
        UILayer_Update(l);
    }
}

void UI_Print_Screen(UI *ui, UISurface *s, Color_Bzzt fg, Color_Bzzt bg, bool wrap, int x, int y, char *fmt, ...)
{
    if (!ui || ui->layer_count == 0)
    {
        Debug_Printf(LOG_UI, "UI is null or UI has no layers when trying to print to screen.");
        return;
    }

    UILayer *layer = ui->layers[0];
    if (layer->surface_count == 0)
        return;

    // UISurface *surface = layer->surfaces[0];
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    UISurface_DrawText(s, buffer, x, y, fg, bg, wrap, s->properties.w);
}

/** old plascii import code */
static cJSON *plascii_load(const char *path)
{
    char *text = read_file(path);
    if (!text)
        return NULL;

    cJSON *json = cJSON_Parse(text);
    if (!json)
    {
        Debug_Printf(LOG_UI, "JSON error before: %s", cJSON_GetErrorPtr());
        free(text);
        return NULL;
    }
    free(text);

    return json;
}

static void playscii_unload(PlaysciiAsset *asset)
{
    free(asset);
}

UISurface *UISurface_Load_From_Playscii(const char *path)
{
    cJSON *json = plascii_load(path);
    CHECK_FAIL(json, fail, "Failure parsing JSON.");

    cJSON *width = cJSON_GetObjectItemCaseSensitive(json, "width");
    CHECK_FAIL(width, fail, "Fail reading width from JSON.");

    cJSON *height = cJSON_GetObjectItemCaseSensitive(json, "height");
    CHECK_FAIL(height, fail, "Fail reading height from JSON.");

    cJSON *frames = cJSON_GetObjectItemCaseSensitive(json, "frames");
    CHECK_FAIL(frames, fail, "Fail reading frames from JSON.");

    cJSON *frame = cJSON_GetArrayItem(frames, 0);
    CHECK_FAIL(frame, fail, "Fail reading frame from JSON.");

    cJSON *layers = cJSON_GetObjectItemCaseSensitive(frame, "layers");
    CHECK_FAIL(layers, fail, "Fail reading layers from JSON.");

    cJSON *layer = cJSON_GetArrayItem(layers, 0);
    CHECK_FAIL(layer, fail, "Fail reading layer from JSON.");

    cJSON *tiles = cJSON_GetObjectItemCaseSensitive(layer, "tiles");
    CHECK_FAIL(tiles, fail, "Fail reading tiles from JSON.");

    int cell_count = cJSON_GetArraySize(tiles);

    UISurface *surface = UISurface_Create(NULL, NULL, 0, true, true, 0, 0, 0,
                                          width->valueint, height->valueint);

    cJSON *tile = NULL;
    int i = 0;
    cJSON_ArrayForEach(tile, tiles)
    {
        cJSON *bg = cJSON_GetObjectItemCaseSensitive(tile, "bg");
        cJSON *glyph = cJSON_GetObjectItemCaseSensitive(tile, "char");
        cJSON *fg = cJSON_GetObjectItemCaseSensitive(tile, "fg");
        surface->cells[i].bg = bzzt_get_color(-1 + bg->valueint);
        surface->cells[i].glyph = glyph->valueint;
        surface->cells[i].fg = bzzt_get_color(-1 + fg->valueint);
        surface->cells[i].visible = true;
        i++;
    }

    surface->properties.w = width->valueint;
    surface->properties.h = height->valueint;

    cJSON_Delete(json);
    return surface;

fail:
    cJSON_Delete(json);
    return NULL;
}
/** -------------- */