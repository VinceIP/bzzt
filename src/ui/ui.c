#include "ui.h"
#include "cJSON.h"
#include "cell.h"
#include "color.h"
#include "ui_layer.h"
#include "ui_surface.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK_FAIL(cond, label, msg)                            \
    do                                                          \
    {                                                           \
        if (!(cond))                                            \
        {                                                       \
            fprintf(stderr, "UISurface load error: %s\n", msg); \
            goto label;                                         \
        }                                                       \
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

UI *UI_Create(void)
{
    UI *ui = malloc(sizeof(UI));
    ui->layer_count = 0;
    ui->layer_cap = 4;
    ui->layers = malloc(sizeof(UILayer *) * ui->layer_cap);
    ui->visible = false;
    return ui;
}

void UI_Destroy(UI *ui)
{
    int layer_count = ui->layer_count;
    for (int i = 0; i < layer_count; ++i)
    {
        UILayer *l = ui->layers[i];
        UILayer_Destroy(l);
    }
    free(ui->layers);
    free(ui);
}

cJSON *Playscii_Load(const char *path)
{
    char *text = read_file(path);
    if (!text)
        return NULL;

    cJSON *json = cJSON_Parse(text);
    if (!json)
    {
        fprintf(stderr, "JSON error before: %s\n", cJSON_GetErrorPtr());
        free(text);
        return NULL;
    }
    free(text);

    return json;
}

void Playscii_Unload(PlaysciiAsset *asset)
{
    free(asset);
}

UISurface *UISurface_Load_From_Playscii(const char *path)
{
    cJSON *json = Playscii_Load(path);
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

    UISurface *surface = UISurface_Create(cell_count);

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
        i++;
    }

    surface->w = width->valueint;
    surface->w = height->valueint;

    cJSON_Delete(json);
    return surface;

fail:
    cJSON_Delete(json);
    return NULL;
}

void UI_Add_Surface(UI *ui, UISurface *s)
{
    (void)ui;
    (void)s;
    // TODO: Implement layered ui surface management
}