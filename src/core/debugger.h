#pragma once
#include <stdbool.h>
#include <stdlib.h>

typedef enum
{
    LOG_NULL,
    LOG_ENGINE,
    LOG_WORLD,
    LOG_BOARD,
    LOG_UI
} LogType;

typedef struct
{
    bool enabled;
} Debugger;

void Debugger_Create();
void Debug_Printf(LogType et, char *fmt, ...);