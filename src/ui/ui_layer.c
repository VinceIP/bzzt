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
        return;
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

    UISurface *surfaces = l->surfaces;
    int surfaces_count = l->surface_count;
}