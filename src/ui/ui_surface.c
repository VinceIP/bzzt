#include "ui_surface.h"
#include "cell.h"
#include <stdlib.h>
#include <stdio.h>

UISurface *UISurface_Create(int cell_count)
{
    UISurface *surface = malloc(sizeof(UISurface));
    if (!surface)
        goto fail;

    surface->visible = false;
    surface->x = surface->y = surface->z = 0;
    surface->w = surface->h = 0;
    surface->cells = malloc(sizeof(Cell) * cell_count);
    if (!surface->cells)
        goto fail;
    surface->cell_count = cell_count;
    surface->elements = NULL;
    surface->elements_count = 0;
    surface->elements_cap = 0;
    return surface;

fail:
    fprintf(stderr, "Error creating surface with %d cells.", cell_count);
    return NULL;
}

void UISurface_Destroy(UISurface *s)
{
    int elements_count = s->elements_count;
    for (int i = 0; i < elements_count; ++i)
    {
        UIElement *e = s->elements[i];
        UIElement_Destroy(e);
    }
    free(s->elements);
    free(s);
}