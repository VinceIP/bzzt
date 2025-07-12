#include "ui_layer.h"
#include "ui_surface.h"
#include <stdlib.h>
#include <stdio.h>

UILayer *UILayer_Create()
{
    UILayer *l = malloc(sizeof(UILayer));
    if (!l)
    {
        fprintf(stderr, "Error allocationg UILayer.");
        return NULL;
    }
    l->surfaces = NULL;
    l->visible = true;
    l->z = 0;
    l->surface_count = 0;
    l->surface_cap = 0;
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

UISurface *UILayer_Add_Surface(UILayer *l, int w, int h, int x, int y)
{
    if (!l)
        return NULL;

    if (l->surface_count >= l->surface_cap && grow_layer(l) != 0)
        return NULL;

    UISurface *s = UISurface_Create(w * h);
    if (!s)
        return NULL;

    s->w = w;
    s->h = h;
    s->x = x;
    s->y = y;

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