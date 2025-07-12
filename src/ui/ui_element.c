#include "ui_element.h"
#include <stdlib.h>

UIElement *UIElement_Create()
{
    return NULL;
}

void UIElement_Destroy(UIElement *e)
{
    free(e);
}