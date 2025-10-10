/**
 * @file ui_bindings.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-07-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui.h"
#include "debugger.h"
#include <stdlib.h>
#include <stdio.h>

TextBinding *UIBinding_Text_Create(const void *ptr, const char *fmt, BindType type)
{
    TextBinding *b = malloc(sizeof *b);
    if (!b)
        return NULL;
    b->ptr = ptr;
    b->fmt = fmt;
    b->type = type;
    b->buf[0] = '\0';
    b->on_heap = true;
    return b;
}

const char *UIBinding_Text_Format(void *ud)
{
    TextBinding *b = ud;
    switch (b->type)
    {
    case BIND_INT:
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%d", *(const int *)b->ptr); // Format the data if it exists and store it into b->buf
        break;
    case BIND_INT16:
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%d", (int)*(const int16_t *)b->ptr);
        break;
    case BIND_FLOAT:
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%.2f", *(const float *)b->ptr);
        break;
    case BIND_STR:
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%s", (const char *)b->ptr);
    }
    return b->buf;
}

void UIText_Rebind_To_Data(UIElement_Text *text, const void *ptr, const char *fmt, BindType type)
{
    if (!text)
        return;

    if (text->owns_ud && text->ud)
        free(text->ud);

    TextBinding *binding = UIBinding_Text_Create(ptr, fmt, type);
    if (!binding)
        return;

    text->textCallback = UIBinding_Text_Format;
    text->ud = binding;
    text->owns_ud = true;
}

const char *pass_through_caption(void *ud) { return (const char *)ud; }