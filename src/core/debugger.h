/**
 * @file debugger.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef enum
{
    LOG_NULL,
    LOG_ENGINE,
    LOG_WORLD,
    LOG_BOARD,
    LOG_UI
} LogType;

typedef enum
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
} LogLevel;

typedef struct
{
    FILE *file;
    char *path;
    int log_limit;
    bool enabled;
    bool log_to_file;
    LogLevel log_level;
} Debugger;

void Debugger_Create();
void Debug_Printf(LogType lt, const char *fmt, ...);
const char *Debug_Log(LogLevel lvl, LogType lt, const char *fmt, ...);