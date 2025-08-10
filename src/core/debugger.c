/**
 * @file debugger.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#define _POSIX_C_SOURCE 200809L
#include "debugger.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdint.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#define PATH_MAX MAX_PATH
#else
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#endif

static Debugger *dbg = NULL;

static void get_time(struct tm *out_tm, time_t *out_sec)
{
    time_t now = time(NULL);
#ifdef _WIN32
    localtime_s(out_tm, &now);
#else
    localtime_r(&now, out_tm);
#endif
    if (out_sec)
        *out_sec = now;
}

static void enforce_log_limit(int limit)
{
    if (limit <= 0)
        return;

    typedef struct
    {
        char name[PATH_MAX];
        time_t mtime;
    } LogEntry;
    LogEntry entries[64];
    size_t count = 0;

#if defined(_WIN32)
    struct _finddata_t f;
    intptr_t h;
    if ((h = _findfirst("bzzt_*.log", &f)) != -1)
    {
        do
        {
            if (count < 64)
            {
                strncpy(entries[count].name, f.name, sizeof entries[0].name);
                entries[count].mtime = f.time_write;
                ++count;
            }
        } while (_findnext(h, &f) == 0);
        _findclose(h);
    }
#else
    DIR *dir = opendir(".");
    if (dir)
    {
        struct dirent *de;
        while ((de = readdir(dir)) != NULL)
        {
            if (strncmp(de->d_name, "bzzt_", 5) == 0)
            {
                size_t len = strlen(de->d_name);
                if (len > 4 && strcmp(de->d_name + len - 4, ".log") == 0)
                {
                    if (count < 64)
                    {
                        strcpy(entries[count].name, de->d_name);
                        struct stat st;
                        if (stat(de->d_name, &st) == 0)
                            entries[count].mtime = st.st_mtime;
                        else
                            entries[count].mtime = 0;
                        ++count;
                    }
                }
            }
        }
        closedir(dir);
    }
#endif

    if (count <= (size_t)limit)
        return;

    for (size_t i = 0; i < count; ++i)
        for (size_t j = i + 1; j < count; ++j)
            if (entries[i].mtime > entries[j].mtime)
            {
                LogEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }

    for (size_t i = 0; i < count - (size_t)limit; ++i)
        remove(entries[i].name);
}

void Debugger_Create(void)
{
    const int log_limit = 5;
    Debugger *d = calloc(1, sizeof *d);
    if (!d)
    {
        fprintf(stderr, "Debugger: malloc failed\n");
        return;
    }

    d->enabled = true;
    d->log_to_file = true;
    d->log_level = LOG_LEVEL_DEBUG;
    d->log_limit = log_limit;

    enforce_log_limit(log_limit);
    struct tm tm_now;
    get_time(&tm_now, NULL);
    char stamp[20];
    strftime(stamp, sizeof stamp, "%Y-%m-%d_%H-%M-%S", &tm_now);

    char fname[PATH_MAX];
    snprintf(fname, sizeof fname, "bzzt_%s.log", stamp);
    d->file = fopen(fname, "w");
    if (!d->file)
    {
        fprintf(stderr, "Debugger: cannot open '%s'\n", fname);
        free(d);
        return;
    }

    strncpy(d->path, fname, sizeof d->path);
    d->path[sizeof d->path - 1] = '\0';

    dbg = d;

    fprintf(dbg->file, "=== bzzt log started %s ===\n", stamp);
    fflush(dbg->file);
}

void Debugger_Destroy(void)
{
    if (!dbg)
        return;
    if (dbg->file)
        fclose(dbg->file);
    free(dbg);
    dbg = NULL;
}

static void write_line(const char *prefix, const char *fmt, va_list ap)
{
    va_list cp;
    va_copy(cp, ap);

    fprintf(stderr, "%s", prefix);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);

    if (dbg && dbg->log_to_file && dbg->file)
    {
        fprintf(dbg->file, "%s", prefix);
        vfprintf(dbg->file, fmt, cp);
        fprintf(dbg->file, "\n");
        fflush(dbg->file);
    }

    va_end(cp);
}

void Debug_Printf(LogType lt, const char *fmt, ...)
{
    if (!dbg || !dbg->enabled)
        return;

    const char *type_str =
        lt == LOG_ENGINE ? "ENGINE: " : lt == LOG_WORLD ? "WORLD: "
                                    : lt == LOG_BOARD   ? "BOARD: "
                                    : lt == LOG_UI      ? "UI: "
                                    : lt == LOG_EDITOR  ? "EDITOR: "
                                                        : "";

    va_list ap;
    va_start(ap, fmt);
    write_line(type_str, fmt, ap);
    va_end(ap);
}

const char *Debug_Log(LogLevel lvl, LogType lt, const char *fmt, ...)
{
    if (!dbg || !dbg->enabled)
        return NULL;
    if (lvl < dbg->log_level)
        return NULL;

    /* timestamp */
    struct tm tm_now;
    get_time(&tm_now, NULL);
    char tbuf[9];
    strftime(tbuf, sizeof tbuf, "%H:%M:%S", &tm_now);

    const char *lvl_s =
        lvl == LOG_LEVEL_ERROR ? "ERROR" : lvl == LOG_LEVEL_WARN ? "WARN"
                                                                 : "DEBUG";

    const char *typ_s =
        lt == LOG_ENGINE ? "ENGINE" : lt == LOG_WORLD ? "WORLD"
                                  : lt == LOG_BOARD   ? "BOARD"
                                  : lt == LOG_UI      ? "UI"
                                                      : "";

    static char prefix[64];
    snprintf(prefix, sizeof prefix, "[%s] %s %s: ", tbuf, lvl_s, typ_s);

    va_list ap;
    va_start(ap, fmt);
    write_line(prefix, fmt, ap);
    va_end(ap);

    return prefix;
}
