#include "debugger.h"
#include <stdarg.h>
#include <stdio.h>

static Debugger *dbg = NULL;

void Debugger_Create()
{
    Debugger *d = malloc(sizeof(Debugger));
    d->enabled = true;
    if (!d)
    {
        fprintf(stderr, "Error creating debugger.");
        return;
    }
    dbg = d;
}

void Debug_Printf(LogType lt, char *fmt, ...)
{
    if (dbg->enabled)
    {
        const char *prefix = "";

        switch (lt)
        {
        case LOG_ENGINE:
            prefix = "ENGINE: ";
            break;
        case LOG_WORLD:
            prefix = "WORLD: ";
            break;
        case LOG_BOARD:
            prefix = "BOARD: ";
            break;
        case LOG_UI:
            prefix = "UI: ";
            break;
        case LOG_NULL:
        default:
            break;
        }

        fprintf(stderr, "%s", prefix);

        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}