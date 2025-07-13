/**
 * @file ui_bindings.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui_bindings.h"
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
    return b;
}

const char *UIBinding_Text_Format(void *ud)
{
    TextBinding *b = ud;
    switch (b->type)
    {
    case BIND_INT:
        Debug_Printf(LOG_UI, "binding int");
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%d", *(const int *)b->ptr); // Format the data if it exists and store it into b->buf
        break;
    case BIND_FLOAT:
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%.2f", *(const float *)b->ptr);
        break;
    case BIND_STR:
        snprintf(b->buf, sizeof b->buf, b->fmt ? b->fmt : "%s", (const char *)b->ptr);
    }
    Debug_Printf(LOG_UI, "Formatted buffer: %s", b->buf);
    return b->buf;
}

static const char *pass_through_caption(void *ud) { return (const char *)ud; }