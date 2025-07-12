#include "ui_element.h"
#include <stdlib.h>

void UIElement_Destroy(UIElement *e)
{
    free(e);
}