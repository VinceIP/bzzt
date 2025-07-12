#include "ui_surface.h"
#include "ui_element.h"
#include <stdlib.h>

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