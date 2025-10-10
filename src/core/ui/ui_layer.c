/**
 * @file ui_layer.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-10
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui.h"
#include "debugger.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// fix this
UILayer *UILayer_Create(bool visible, bool enabled)
{
    UILayer *l = calloc(1, sizeof(UILayer));
    if (!l)
    {
        Debug_Printf(LOG_UI, "Error allocating UILayer.");
        return NULL;
    }
    l->visible = visible;
    l->enabled = enabled;
    l->id = 0;
    l->name = NULL;
    l->surface_cap = 1;
    l->surface_count = 0;
    l->surfaces = malloc(sizeof(UISurface *) * l->surface_cap);
    if (!l->surfaces)
    {
        free(l);
        Debug_Printf(LOG_UI, "Error allocating surfaces array for UILayer.");
        return NULL;
    }
    l->index = -1;
    return l;
}

void UILayer_Destroy(UILayer *l)
{
    if (!l)
        return;

    for (int i = 0; i < l->surface_count; ++i)
    {
        UISurface *s = l->surfaces[i];
        UISurface_Destroy(s);
    }

    free(l->surfaces);
    free(l);
}

static int grow_layer(UILayer *l)
{
    int new_cap = l->surface_cap == 0 ? 4 : l->surface_cap * 2;
    UISurface **tmp = realloc(l->surfaces, sizeof(UISurface *) * new_cap);
    if (!tmp)
        return -1;
    l->surfaces = tmp;
    l->surface_cap = new_cap;
    return 0;
}

UISurface *UILayer_Add_New_Surface(UILayer *l, char *name, int id, bool visible, bool enabled, int x, int y, int z, int w, int h)
{
    if (!l)
        return NULL;

    if (l->surface_count >= l->surface_cap && grow_layer(l) != 0)
        return NULL;

    UISurface *s = UISurface_Create(l, name, id, visible, enabled, x, y, z, w, h);
    if (!s)
        return NULL;

    l->surfaces[l->surface_count++] = s;
    return s;
}

void UILayer_Update(UILayer *l)
{
    if (!l || !l->visible)
        return;

    for (int i = 0; i < l->surface_count; ++i)
    {
        UISurface *s = l->surfaces[i];
        UISurface_Update(s);
    }
}

void UILayer_Set_Enabled(UILayer *layer, bool enabled)
{
    if (!layer)
        return;
    layer->enabled = enabled;
}
void UILayer_Set_Visible(UILayer *layer, bool visible)
{
    if (!layer)
        return;
    layer->visible = visible;
}