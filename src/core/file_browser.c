#include "file_browser.h"

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define FILE_BROWSER_STATUS_MAX 192
#define FILE_BROWSER_LIST_BUFFER_MAX 8192
#define FILE_BROWSER_PATH_BUFFER_MAX 512
#define FILE_BROWSER_DEFAULT_VISIBLE_ROWS 11

typedef enum FileBrowserEntryType
{
    FILE_BROWSER_ENTRY_PARENT = 0,
    FILE_BROWSER_ENTRY_DIRECTORY,
    FILE_BROWSER_ENTRY_ZZT,
    FILE_BROWSER_ENTRY_BZZT,
    FILE_BROWSER_ENTRY_OTHER
} FileBrowserEntryType;

typedef struct FileBrowserEntry
{
    char *name;
    FileBrowserEntryType type;
} FileBrowserEntry;

struct FileBrowser
{
    char current_dir[PATH_MAX];
    FileBrowserEntry *entries;
    int entries_count;
    int entries_cap;
    int selected_index;
    int scroll_offset;
    int visible_rows;
    char status[FILE_BROWSER_STATUS_MAX];
    char list_buffer[FILE_BROWSER_LIST_BUFFER_MAX];
    char path_buffer[FILE_BROWSER_PATH_BUFFER_MAX];
};

static void file_browser_free_entries(FileBrowser *browser)
{
    if (!browser || !browser->entries)
        return;

    for (int i = 0; i < browser->entries_count; ++i)
    {
        free(browser->entries[i].name);
        browser->entries[i].name = NULL;
    }

    free(browser->entries);
    browser->entries = NULL;
    browser->entries_count = 0;
    browser->entries_cap = 0;
}

static bool file_browser_reserve(FileBrowser *browser, int needed)
{
    if (!browser)
        return false;

    if (browser->entries_cap >= needed)
        return true;

    int new_cap = browser->entries_cap == 0 ? 32 : browser->entries_cap * 2;
    while (new_cap < needed)
        new_cap *= 2;

    FileBrowserEntry *new_entries = realloc(browser->entries, sizeof(FileBrowserEntry) * new_cap);
    if (!new_entries)
        return false;

    browser->entries = new_entries;
    browser->entries_cap = new_cap;
    return true;
}

static bool file_browser_add_entry(FileBrowser *browser, const char *name, FileBrowserEntryType type)
{
    if (!browser || !name)
        return false;

    if (!file_browser_reserve(browser, browser->entries_count + 1))
        return false;

    browser->entries[browser->entries_count].name = strdup(name);
    if (!browser->entries[browser->entries_count].name)
        return false;

    browser->entries[browser->entries_count].type = type;
    browser->entries_count++;
    return true;
}

static int file_browser_entry_sort_key(FileBrowserEntryType type)
{
    switch (type)
    {
    case FILE_BROWSER_ENTRY_DIRECTORY:
        return 0;
    case FILE_BROWSER_ENTRY_ZZT:
        return 1;
    case FILE_BROWSER_ENTRY_BZZT:
        return 2;
    case FILE_BROWSER_ENTRY_OTHER:
        return 3;
    case FILE_BROWSER_ENTRY_PARENT:
    default:
        return -1;
    }
}

static int file_browser_compare_entries(const void *lhs, const void *rhs)
{
    const FileBrowserEntry *a = (const FileBrowserEntry *)lhs;
    const FileBrowserEntry *b = (const FileBrowserEntry *)rhs;

    int a_key = file_browser_entry_sort_key(a->type);
    int b_key = file_browser_entry_sort_key(b->type);
    if (a_key != b_key)
        return a_key - b_key;

    return strcasecmp(a->name, b->name);
}

static void file_browser_adjust_scroll(FileBrowser *browser)
{
    if (!browser)
        return;

    if (browser->visible_rows <= 0)
        browser->visible_rows = FILE_BROWSER_DEFAULT_VISIBLE_ROWS;

    if (browser->selected_index < browser->scroll_offset)
        browser->scroll_offset = browser->selected_index;

    if (browser->selected_index >= browser->scroll_offset + browser->visible_rows)
        browser->scroll_offset = browser->selected_index - browser->visible_rows + 1;

    if (browser->scroll_offset < 0)
        browser->scroll_offset = 0;
}

static bool file_browser_join_path(char *dst, size_t dst_size, const char *dir, const char *name)
{
    if (!dst || dst_size == 0 || !dir || !name)
        return false;

    if (strcmp(dir, "/") == 0)
        return snprintf(dst, dst_size, "/%s", name) < (int)dst_size;

    return snprintf(dst, dst_size, "%s/%s", dir, name) < (int)dst_size;
}

static bool file_browser_normalize_directory(const char *path, char *out_dir, size_t out_dir_size)
{
    if (!path || !out_dir || out_dir_size == 0)
        return false;

    if (snprintf(out_dir, out_dir_size, "%s", path) >= (int)out_dir_size)
        return false;

    size_t len = strlen(out_dir);
    while (len > 1 && out_dir[len - 1] == '/')
    {
        out_dir[len - 1] = '\0';
        len--;
    }

    return true;
}

static bool file_browser_parent_directory(const char *dir, char *out_dir, size_t out_dir_size)
{
    if (!dir || !out_dir || out_dir_size == 0)
        return false;

    if (!file_browser_normalize_directory(dir, out_dir, out_dir_size))
        return false;

    if (strcmp(out_dir, "/") == 0)
        return snprintf(out_dir, out_dir_size, "/") < (int)out_dir_size;

    char *slash = strrchr(out_dir, '/');
    if (!slash)
        return snprintf(out_dir, out_dir_size, ".") < (int)out_dir_size;

    if (slash == out_dir)
    {
        out_dir[1] = '\0';
        return true;
    }

    *slash = '\0';
    return true;
}

static bool file_browser_path_is_directory(const char *path)
{
    if (!path)
        return false;
    struct stat st = {0};
    if (stat(path, &st) != 0)
        return false;

    return S_ISDIR(st.st_mode);
}

static FileBrowserEntryType file_browser_entry_type(const char *dir, const struct dirent *entry)
{
    const char *name = entry->d_name;
    const char *ext = strrchr(name, '.');
    char full_path[PATH_MAX] = {0};

    if (file_browser_join_path(full_path, sizeof(full_path), dir, name) &&
        file_browser_path_is_directory(full_path))
        return FILE_BROWSER_ENTRY_DIRECTORY;

    if (!ext)
        return FILE_BROWSER_ENTRY_OTHER;

    if (strcasecmp(ext, ".zzt") == 0)
        return FILE_BROWSER_ENTRY_ZZT;

    if (strcasecmp(ext, ".bzzt") == 0)
        return FILE_BROWSER_ENTRY_BZZT;

    return FILE_BROWSER_ENTRY_OTHER;
}

static bool file_browser_scan(FileBrowser *browser, const char *dir)
{
    if (!browser || !dir)
        return false;

    char resolved_dir[PATH_MAX] = {0};
    if (!file_browser_normalize_directory(dir, resolved_dir, sizeof(resolved_dir)))
    {
        FileBrowser_SetStatus(browser, "Unable to open that directory.");
        return false;
    }

    DIR *dp = opendir(resolved_dir);
    if (!dp)
    {
        FileBrowser_SetStatus(browser, "Unable to open that directory.");
        return false;
    }

    file_browser_free_entries(browser);

    if (!file_browser_add_entry(browser, "../", FILE_BROWSER_ENTRY_PARENT))
    {
        closedir(dp);
        return false;
    }

    struct dirent *entry = NULL;
    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        FileBrowserEntryType type = file_browser_entry_type(resolved_dir, entry);
        if (!file_browser_add_entry(browser, entry->d_name, type))
        {
            closedir(dp);
            return false;
        }
    }

    closedir(dp);

    if (browser->entries_count > 1)
    {
        qsort(browser->entries + 1,
              (size_t)(browser->entries_count - 1),
              sizeof(FileBrowserEntry),
              file_browser_compare_entries);
    }

    snprintf(browser->current_dir, sizeof(browser->current_dir), "%s", resolved_dir);
    browser->selected_index = 0;
    browser->scroll_offset = 0;
    FileBrowser_ClearStatus(browser);
    return true;
}

static void file_browser_format_name(char *dst,
                                     size_t dst_size,
                                     const FileBrowserEntry *entry,
                                     int max_chars)
{
    if (!dst || dst_size == 0 || !entry || max_chars <= 0)
        return;

    const char *prefix = " ";
    const char *color = "\\f8";

    switch (entry->type)
    {
    case FILE_BROWSER_ENTRY_PARENT:
    case FILE_BROWSER_ENTRY_DIRECTORY:
        color = "\\f14";
        break;
    case FILE_BROWSER_ENTRY_ZZT:
        color = "\\f15";
        break;
    case FILE_BROWSER_ENTRY_BZZT:
    case FILE_BROWSER_ENTRY_OTHER:
        color = "\\f7";
        break;
    }

    int written = snprintf(dst, dst_size, "%s%s", color, prefix);
    if (written < 0 || written >= (int)dst_size)
        return;

    int remaining = max_chars;
    const char *name = entry->name;
    while (*name && remaining > 0 && written < (int)dst_size - 1)
    {
        char ch = *name++;
        if (ch == '\\' || (unsigned char)ch < 32)
            ch = '?';
        dst[written++] = ch;
        remaining--;
    }

    if (entry->type == FILE_BROWSER_ENTRY_DIRECTORY &&
        written < (int)dst_size - 1)
    {
        dst[written++] = '/';
    }

    dst[written] = '\0';
}

FileBrowser *FileBrowser_Create(void)
{
    FileBrowser *browser = calloc(1, sizeof(FileBrowser));
    if (!browser)
        return NULL;

    browser->visible_rows = FILE_BROWSER_DEFAULT_VISIBLE_ROWS;
    return browser;
}

void FileBrowser_Destroy(FileBrowser *browser)
{
    if (!browser)
        return;

    file_browser_free_entries(browser);
    free(browser);
}

bool FileBrowser_Open(FileBrowser *browser, const char *start_dir)
{
    if (!browser || !start_dir)
        return false;

    return file_browser_scan(browser, start_dir);
}

bool FileBrowser_MoveSelection(FileBrowser *browser, int delta)
{
    if (!browser || browser->entries_count <= 0 || delta == 0)
        return false;

    int next = browser->selected_index + delta;
    if (next < 0)
        next = 0;
    if (next >= browser->entries_count)
        next = browser->entries_count - 1;

    if (next == browser->selected_index)
        return false;

    browser->selected_index = next;
    browser->status[0] = '\0';
    file_browser_adjust_scroll(browser);
    return true;
}

bool FileBrowser_MovePage(FileBrowser *browser, int page_delta)
{
    if (!browser || page_delta == 0)
        return false;

    int page_size = browser->visible_rows > 0 ? browser->visible_rows : FILE_BROWSER_DEFAULT_VISIBLE_ROWS;
    return FileBrowser_MoveSelection(browser, page_delta * page_size);
}

void FileBrowser_SetVisibleRows(FileBrowser *browser, int rows)
{
    if (!browser)
        return;

    browser->visible_rows = rows > 0 ? rows : FILE_BROWSER_DEFAULT_VISIBLE_ROWS;
    file_browser_adjust_scroll(browser);
}

void FileBrowser_SetStatus(FileBrowser *browser, const char *status)
{
    if (!browser)
        return;

    if (!status)
    {
        browser->status[0] = '\0';
        return;
    }

    snprintf(browser->status, sizeof(browser->status), "%s", status);
}

void FileBrowser_ClearStatus(FileBrowser *browser)
{
    FileBrowser_SetStatus(browser, NULL);
}

FileBrowserActivateResult FileBrowser_Activate(FileBrowser *browser,
                                               char *out_path,
                                               size_t out_path_size)
{
    if (!browser || browser->entries_count <= 0)
        return FILE_BROWSER_ACTIVATE_NONE;

    FileBrowserEntry *entry = &browser->entries[browser->selected_index];
    char path[PATH_MAX] = {0};

    if (entry->type == FILE_BROWSER_ENTRY_PARENT)
    {
        if (!file_browser_parent_directory(browser->current_dir, path, sizeof(path)))
            return FILE_BROWSER_ACTIVATE_NONE;

        if (!file_browser_scan(browser, path))
            return FILE_BROWSER_ACTIVATE_NONE;

        return FILE_BROWSER_ACTIVATE_NAVIGATED;
    }

    if (!file_browser_join_path(path, sizeof(path), browser->current_dir, entry->name))
    {
        FileBrowser_SetStatus(browser, "Path is too long.");
        return FILE_BROWSER_ACTIVATE_NONE;
    }

    if (entry->type == FILE_BROWSER_ENTRY_DIRECTORY)
    {
        if (!file_browser_scan(browser, path))
            return FILE_BROWSER_ACTIVATE_NONE;

        return FILE_BROWSER_ACTIVATE_NAVIGATED;
    }

    if (entry->type != FILE_BROWSER_ENTRY_ZZT)
    {
        FileBrowser_SetStatus(browser, "Only .zzt files can be opened right now.");
        return FILE_BROWSER_ACTIVATE_NONE;
    }

    if (!out_path || out_path_size == 0)
        return FILE_BROWSER_ACTIVATE_NONE;

    if (snprintf(out_path, out_path_size, "%s", path) >= (int)out_path_size)
    {
        FileBrowser_SetStatus(browser, "Path is too long.");
        return FILE_BROWSER_ACTIVATE_NONE;
    }

    FileBrowser_ClearStatus(browser);
    return FILE_BROWSER_ACTIVATE_SELECTED_WORLD;
}

const char *FileBrowser_FormatDirectory(void *ud)
{
    FileBrowser *browser = (FileBrowser *)ud;
    if (!browser)
        return "";

    const int max_len = FILE_BROWSER_PATH_BUFFER_MAX - 16;
    const char *dir = browser->current_dir[0] ? browser->current_dir : ".";
    size_t len = strlen(dir);

    if ((int)len <= max_len)
    {
        snprintf(browser->path_buffer,
                 sizeof(browser->path_buffer),
                 "\\f15Dir: %.*s",
                 max_len,
                 dir);
        return browser->path_buffer;
    }

    const char *tail = dir + len - (size_t)(max_len - 3);
    snprintf(browser->path_buffer, sizeof(browser->path_buffer), "\\f15Dir: ...%s", tail);
    return browser->path_buffer;
}

const char *FileBrowser_FormatEntries(void *ud)
{
    FileBrowser *browser = (FileBrowser *)ud;
    if (!browser)
        return "";

    browser->list_buffer[0] = '\0';

    if (browser->entries_count <= 0)
    {
        snprintf(browser->list_buffer, sizeof(browser->list_buffer), "\\f8  (empty)");
        return browser->list_buffer;
    }

    int start = browser->scroll_offset;
    int end = start + browser->visible_rows;
    if (end > browser->entries_count)
        end = browser->entries_count;

    for (int i = start; i < end; ++i)
    {
        const FileBrowserEntry *entry = &browser->entries[i];
        char line[256] = {0};
        char name[192] = {0};
        const bool selected = i == browser->selected_index;

        file_browser_format_name(name, sizeof(name), entry, 48);

        snprintf(line,
                 sizeof(line),
                 "%s%s%s",
                 selected ? "\\b1\\f15> " : "",
                 selected ? "" : "  ",
                 name);

        strncat(browser->list_buffer,
                line,
                sizeof(browser->list_buffer) - strlen(browser->list_buffer) - 1);

        if (i < end - 1)
        {
            strncat(browser->list_buffer,
                    "\n",
                    sizeof(browser->list_buffer) - strlen(browser->list_buffer) - 1);
        }
    }

    return browser->list_buffer;
}

const char *FileBrowser_FormatStatus(void *ud)
{
    FileBrowser *browser = (FileBrowser *)ud;
    if (!browser)
        return "";

    if (browser->status[0] != '\0')
        return browser->status;

    return "\\f8Directories are yellow. Only .zzt files open.";
}
