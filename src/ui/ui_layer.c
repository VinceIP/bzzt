#include "ui_layer.h"
#include "ui_surface.h"
#include <stdlib.h>

void UILayer_Destroy(UILayer *l)
{
    int surface_count = l->surface_count;
    for (int i = 0; i < surface_count; ++i)
    {
        UISurface *s = l->surfaces[i];
        UISurface_Destroy(s);
    }
    free(l->surfaces);
    free(l);
}