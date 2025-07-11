#include "raylib.h"
#include "cJSON.h"
#include <stdint.h>

typedef struct
{
    int width, height;
    uint32_t *glyphs;
    uint8_t *fg, *bg;
    Color palette[32];
} PlaysciiAsset;

typedef struct
{
    PlaysciiAsset *asset;
    char **dyn_glyphs;
    uint8_t *dyn_fg, *dyn_bg;
} UISurface;

PlaysciiAsset *Load_Playscii(const char *path);
void UnloadPlayscii(PlaysciiAsset *asset, cJSON *json);