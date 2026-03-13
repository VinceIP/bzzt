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

#include "zip_archive.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define FILE_BROWSER_STATUS_MAX 192
#define FILE_BROWSER_LIST_BUFFER_MAX 8192
#define FILE_BROWSER_PATH_BUFFER_MAX 768
#define FILE_BROWSER_DISPLAY_PATH_MAX 2048
#define FILE_BROWSER_DEFAULT_VISIBLE_ROWS 11
#define FILE_BROWSER_MAX_ZIP_DEPTH 8

typedef enum FileBrowserMode
{
    FILE_BROWSER_MODE_FILESYSTEM = 0,
    FILE_BROWSER_MODE_ZIP
} FileBrowserMode;

typedef enum FileBrowserEntryType
{
    FILE_BROWSER_ENTRY_PARENT = 0,
    FILE_BROWSER_ENTRY_DIRECTORY,
    FILE_BROWSER_ENTRY_ZIP,
    FILE_BROWSER_ENTRY_ZZT,
    FILE_BROWSER_ENTRY_BZZT,
    FILE_BROWSER_ENTRY_OTHER
} FileBrowserEntryType;

typedef struct FileBrowserEntry
{
    char *name;
    char *path;
    FileBrowserEntryType type;
} FileBrowserEntry;

typedef struct FileBrowserZipContext
{
    char archive_path[PATH_MAX];
    char display_root[FILE_BROWSER_DISPLAY_PATH_MAX];
    char member_from_parent[PATH_MAX];
    char zip_dir[PATH_MAX];
    bool owns_temp_archive;
    struct FileBrowserZipContext *parent;
} FileBrowserZipContext;

typedef struct FileBrowserSavedLocation
{
    bool valid;
    char current_dir[PATH_MAX];
    int zip_depth;
    char top_archive_path[PATH_MAX];
    char zip_dirs[FILE_BROWSER_MAX_ZIP_DEPTH][PATH_MAX];
    char zip_members[FILE_BROWSER_MAX_ZIP_DEPTH - 1][PATH_MAX];
} FileBrowserSavedLocation;

struct FileBrowser
{
    FileBrowserMode mode;
    char current_dir[PATH_MAX];
    FileBrowserZipContext *zip_context;
    FileBrowserEntry *entries;
    int entries_count;
    int entries_cap;
    int selected_index;
    int scroll_offset;
    int visible_rows;
    char status[FILE_BROWSER_STATUS_MAX];
    char list_buffer[FILE_BROWSER_LIST_BUFFER_MAX];
    char path_buffer[FILE_BROWSER_PATH_BUFFER_MAX];
    FileBrowserSavedLocation saved_location;
    FileBrowserSavedLocation pending_location;
};

static bool file_browser_scan_filesystem(FileBrowser *browser, const char *dir);
static bool file_browser_scan_zip(FileBrowser *browser);
static bool file_browser_restore_saved_location(FileBrowser *browser,
                                                const FileBrowserSavedLocation *location);

static void file_browser_copy_text(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0)
        return;

    if (!src)
    {
        dst[0] = '\0';
        return;
    }

    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

static void file_browser_append_text(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0 || !src)
        return;

    size_t len = strlen(dst);
    if (len >= dst_size - 1)
        return;

    strncat(dst, src, dst_size - len - 1);
}

static void file_browser_reset_saved_location(FileBrowserSavedLocation *location)
{
    if (!location)
        return;

    memset(location, 0, sizeof(*location));
}

static FileBrowserZipContext *file_browser_current_zip(FileBrowser *browser)
{
    if (!browser)
        return NULL;
    return browser->zip_context;
}

static void file_browser_free_entries(FileBrowser *browser)
{
    if (!browser || !browser->entries)
        return;

    for (int i = 0; i < browser->entries_count; ++i)
    {
        free(browser->entries[i].name);
        free(browser->entries[i].path);
        browser->entries[i].name = NULL;
        browser->entries[i].path = NULL;
    }

    free(browser->entries);
    browser->entries = NULL;
    browser->entries_count = 0;
    browser->entries_cap = 0;
}

static void file_browser_clear_zip_contexts(FileBrowser *browser)
{
    if (!browser)
        return;

    FileBrowserZipContext *ctx = browser->zip_context;
    while (ctx)
    {
        FileBrowserZipContext *parent = ctx->parent;
        if (ctx->owns_temp_archive)
            unlink(ctx->archive_path);
        free(ctx);
        ctx = parent;
    }

    browser->zip_context = NULL;
    browser->mode = FILE_BROWSER_MODE_FILESYSTEM;
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

    FileBrowserEntry *new_entries = realloc(browser->entries, sizeof(FileBrowserEntry) * (size_t)new_cap);
    if (!new_entries)
        return false;

    browser->entries = new_entries;
    browser->entries_cap = new_cap;
    return true;
}

static bool file_browser_add_entry(FileBrowser *browser,
                                   const char *name,
                                   const char *path,
                                   FileBrowserEntryType type)
{
    if (!browser || !name || !path)
        return false;

    if (!file_browser_reserve(browser, browser->entries_count + 1))
        return false;

    FileBrowserEntry *entry = &browser->entries[browser->entries_count];
    entry->name = strdup(name);
    entry->path = strdup(path);
    if (!entry->name || !entry->path)
    {
        free(entry->name);
        free(entry->path);
        entry->name = NULL;
        entry->path = NULL;
        return false;
    }

    entry->type = type;
    browser->entries_count++;
    return true;
}

static int file_browser_entry_sort_key(FileBrowserEntryType type)
{
    switch (type)
    {
    case FILE_BROWSER_ENTRY_DIRECTORY:
        return 0;
    case FILE_BROWSER_ENTRY_ZIP:
        return 1;
    case FILE_BROWSER_ENTRY_ZZT:
        return 2;
    case FILE_BROWSER_ENTRY_BZZT:
        return 3;
    case FILE_BROWSER_ENTRY_OTHER:
        return 4;
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

static bool file_browser_parent_zip_dir(const char *dir, char *out_dir, size_t out_dir_size)
{
    if (!dir || !out_dir || out_dir_size == 0)
        return false;

    if (snprintf(out_dir, out_dir_size, "%s", dir) >= (int)out_dir_size)
        return false;

    if (out_dir[0] == '\0')
        return true;

    char *slash = strrchr(out_dir, '/');
    if (!slash)
    {
        out_dir[0] = '\0';
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

    if (strcasecmp(ext, ".zip") == 0)
        return FILE_BROWSER_ENTRY_ZIP;
    if (strcasecmp(ext, ".zzt") == 0)
        return FILE_BROWSER_ENTRY_ZZT;
    if (strcasecmp(ext, ".bzzt") == 0)
        return FILE_BROWSER_ENTRY_BZZT;

    return FILE_BROWSER_ENTRY_OTHER;
}

static bool file_browser_push_zip_context(FileBrowser *browser,
                                          const char *archive_path,
                                          const char *display_root,
                                          bool owns_temp_archive,
                                          const char *member_from_parent)
{
    if (!browser || !archive_path || !display_root)
        return false;

    FileBrowserZipContext *ctx = calloc(1, sizeof(FileBrowserZipContext));
    if (!ctx)
        return false;

    if (snprintf(ctx->archive_path, sizeof(ctx->archive_path), "%s", archive_path) >= (int)sizeof(ctx->archive_path) ||
        snprintf(ctx->display_root, sizeof(ctx->display_root), "%s", display_root) >= (int)sizeof(ctx->display_root))
    {
        free(ctx);
        return false;
    }

    if (member_from_parent &&
        snprintf(ctx->member_from_parent, sizeof(ctx->member_from_parent), "%s", member_from_parent) >=
            (int)sizeof(ctx->member_from_parent))
    {
        free(ctx);
        return false;
    }

    ctx->owns_temp_archive = owns_temp_archive;
    ctx->parent = browser->zip_context;
    browser->zip_context = ctx;
    browser->mode = FILE_BROWSER_MODE_ZIP;
    return true;
}

static bool file_browser_pop_zip_context(FileBrowser *browser)
{
    if (!browser || !browser->zip_context)
        return false;

    FileBrowserZipContext *ctx = browser->zip_context;
    browser->zip_context = ctx->parent;
    if (ctx->owns_temp_archive)
        unlink(ctx->archive_path);
    free(ctx);

    browser->mode = browser->zip_context ? FILE_BROWSER_MODE_ZIP : FILE_BROWSER_MODE_FILESYSTEM;
    return true;
}

static bool file_browser_enter_nested_zip(FileBrowser *browser, const FileBrowserEntry *entry)
{
    if (!browser || !entry || entry->type != FILE_BROWSER_ENTRY_ZIP)
        return false;

    FileBrowserZipContext *ctx = file_browser_current_zip(browser);
    if (!ctx)
        return false;

    char temp_path[PATH_MAX] = {0};
    char error[FILE_BROWSER_STATUS_MAX] = {0};
    if (!ZipArchive_Extract_Member_To_Temp_File(ctx->archive_path,
                                                entry->path,
                                                temp_path,
                                                sizeof(temp_path),
                                                error,
                                                sizeof(error)))
    {
        FileBrowser_SetStatus(browser, error[0] ? error : "Unable to open nested zip archive.");
        return false;
    }

    char display_root[FILE_BROWSER_DISPLAY_PATH_MAX] = {0};
    if (snprintf(display_root, sizeof(display_root), "%s!/%s", ctx->display_root, entry->path) >=
        (int)sizeof(display_root))
    {
        unlink(temp_path);
        FileBrowser_SetStatus(browser, "Nested zip path is too long.");
        return false;
    }

    if (!file_browser_push_zip_context(browser, temp_path, display_root, true, entry->path))
    {
        unlink(temp_path);
        FileBrowser_SetStatus(browser, "Out of memory while opening nested zip.");
        return false;
    }

    if (!file_browser_scan_zip(browser))
    {
        file_browser_pop_zip_context(browser);
        return false;
    }

    return true;
}

static bool file_browser_snapshot_location(FileBrowser *browser, FileBrowserSavedLocation *location)
{
    if (!browser || !location)
        return false;

    file_browser_reset_saved_location(location);
    location->valid = true;
    file_browser_copy_text(location->current_dir, sizeof(location->current_dir), browser->current_dir);

    if (browser->mode != FILE_BROWSER_MODE_ZIP || !browser->zip_context)
        return true;

    FileBrowserZipContext *contexts[FILE_BROWSER_MAX_ZIP_DEPTH] = {0};
    int depth = 0;
    for (FileBrowserZipContext *ctx = browser->zip_context; ctx; ctx = ctx->parent)
    {
        if (depth >= FILE_BROWSER_MAX_ZIP_DEPTH)
        {
            file_browser_reset_saved_location(location);
            return false;
        }
        contexts[depth++] = ctx;
    }

    location->zip_depth = depth;
    FileBrowserZipContext *root = contexts[depth - 1];
    file_browser_copy_text(location->top_archive_path,
                           sizeof(location->top_archive_path),
                           root->archive_path);

    for (int i = depth - 1, saved_index = 0; i >= 0; --i, ++saved_index)
    {
        file_browser_copy_text(location->zip_dirs[saved_index],
                               sizeof(location->zip_dirs[saved_index]),
                               contexts[i]->zip_dir);
        if (saved_index > 0)
        {
            file_browser_copy_text(location->zip_members[saved_index - 1],
                                   sizeof(location->zip_members[saved_index - 1]),
                                   contexts[i]->member_from_parent);
        }
    }

    return true;
}

static bool file_browser_restore_saved_location(FileBrowser *browser,
                                                const FileBrowserSavedLocation *location)
{
    if (!browser || !location || !location->valid)
        return false;

    if (location->zip_depth <= 0)
        return file_browser_scan_filesystem(browser, location->current_dir);

    file_browser_free_entries(browser);
    file_browser_clear_zip_contexts(browser);
    file_browser_copy_text(browser->current_dir, sizeof(browser->current_dir), location->current_dir);

    if (!file_browser_push_zip_context(browser,
                                       location->top_archive_path,
                                       location->top_archive_path,
                                       false,
                                       NULL))
    {
        file_browser_clear_zip_contexts(browser);
        return false;
    }

    file_browser_copy_text(browser->zip_context->zip_dir,
                           sizeof(browser->zip_context->zip_dir),
                           location->zip_dirs[0]);

    for (int level = 1; level < location->zip_depth; ++level)
    {
        FileBrowserZipContext *parent = file_browser_current_zip(browser);
        if (!parent)
        {
            file_browser_clear_zip_contexts(browser);
            return false;
        }

        char temp_path[PATH_MAX] = {0};
        char error[FILE_BROWSER_STATUS_MAX] = {0};
        if (!ZipArchive_Extract_Member_To_Temp_File(parent->archive_path,
                                                    location->zip_members[level - 1],
                                                    temp_path,
                                                    sizeof(temp_path),
                                                    error,
                                                    sizeof(error)))
        {
            file_browser_clear_zip_contexts(browser);
            return false;
        }

        char display_root[FILE_BROWSER_DISPLAY_PATH_MAX] = {0};
        file_browser_copy_text(display_root, sizeof(display_root), parent->display_root);
        file_browser_append_text(display_root, sizeof(display_root), "!/");
        file_browser_append_text(display_root,
                                 sizeof(display_root),
                                 location->zip_members[level - 1]);

        if (!file_browser_push_zip_context(browser,
                                           temp_path,
                                           display_root,
                                           true,
                                           location->zip_members[level - 1]))
        {
            unlink(temp_path);
            file_browser_clear_zip_contexts(browser);
            return false;
        }

        file_browser_copy_text(browser->zip_context->zip_dir,
                               sizeof(browser->zip_context->zip_dir),
                               location->zip_dirs[level]);
    }

    if (!file_browser_scan_zip(browser))
    {
        file_browser_free_entries(browser);
        file_browser_clear_zip_contexts(browser);
        return false;
    }

    return true;
}

static bool file_browser_scan_filesystem(FileBrowser *browser, const char *dir)
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
    file_browser_clear_zip_contexts(browser);

    if (!file_browser_add_entry(browser, "../", "../", FILE_BROWSER_ENTRY_PARENT))
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
        if (!file_browser_add_entry(browser, entry->d_name, entry->d_name, type))
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

    browser->mode = FILE_BROWSER_MODE_FILESYSTEM;
    snprintf(browser->current_dir, sizeof(browser->current_dir), "%s", resolved_dir);
    browser->selected_index = 0;
    browser->scroll_offset = 0;
    FileBrowser_ClearStatus(browser);
    return true;
}

static bool file_browser_scan_zip(FileBrowser *browser)
{
    if (!browser)
        return false;

    FileBrowserZipContext *ctx = file_browser_current_zip(browser);
    if (!ctx)
        return false;

    ZipArchiveEntry *zip_entries = NULL;
    int zip_entry_count = 0;
    char error[FILE_BROWSER_STATUS_MAX] = {0};
    if (!ZipArchive_List(ctx->archive_path,
                         ctx->zip_dir,
                         &zip_entries,
                         &zip_entry_count,
                         error,
                         sizeof(error)))
    {
        FileBrowser_SetStatus(browser, error[0] ? error : "Unable to open that zip archive.");
        return false;
    }

    file_browser_free_entries(browser);

    if (!file_browser_add_entry(browser, "../", "../", FILE_BROWSER_ENTRY_PARENT))
    {
        ZipArchive_FreeEntries(zip_entries, zip_entry_count);
        return false;
    }

    for (int i = 0; i < zip_entry_count; ++i)
    {
        FileBrowserEntryType type = FILE_BROWSER_ENTRY_OTHER;
        switch (zip_entries[i].type)
        {
        case ZIP_ARCHIVE_ENTRY_DIRECTORY:
            type = FILE_BROWSER_ENTRY_DIRECTORY;
            break;
        case ZIP_ARCHIVE_ENTRY_ZIP:
            type = FILE_BROWSER_ENTRY_ZIP;
            break;
        case ZIP_ARCHIVE_ENTRY_ZZT:
            type = FILE_BROWSER_ENTRY_ZZT;
            break;
        case ZIP_ARCHIVE_ENTRY_BZZT:
            type = FILE_BROWSER_ENTRY_BZZT;
            break;
        }

        if (!file_browser_add_entry(browser, zip_entries[i].name, zip_entries[i].path, type))
        {
            ZipArchive_FreeEntries(zip_entries, zip_entry_count);
            return false;
        }
    }

    ZipArchive_FreeEntries(zip_entries, zip_entry_count);

    browser->mode = FILE_BROWSER_MODE_ZIP;
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
    case FILE_BROWSER_ENTRY_ZIP:
        color = "\\f11";
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
    browser->mode = FILE_BROWSER_MODE_FILESYSTEM;
    file_browser_reset_saved_location(&browser->saved_location);
    file_browser_reset_saved_location(&browser->pending_location);
    return browser;
}

void FileBrowser_Destroy(FileBrowser *browser)
{
    if (!browser)
        return;

    file_browser_free_entries(browser);
    file_browser_clear_zip_contexts(browser);
    free(browser);
}

bool FileBrowser_Open(FileBrowser *browser, const char *start_dir)
{
    if (!browser || !start_dir)
        return false;

    if (file_browser_restore_saved_location(browser, &browser->saved_location))
        return true;

    return file_browser_scan_filesystem(browser, start_dir);
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

void FileBrowser_RememberPendingLocation(FileBrowser *browser)
{
    if (!browser)
        return;

    if (!file_browser_snapshot_location(browser, &browser->pending_location))
        file_browser_reset_saved_location(&browser->pending_location);
}

void FileBrowser_CommitPendingLocation(FileBrowser *browser)
{
    if (!browser)
        return;

    if (!browser->pending_location.valid)
        return;

    browser->saved_location = browser->pending_location;
    file_browser_reset_saved_location(&browser->pending_location);
}

void FileBrowser_ClearPendingLocation(FileBrowser *browser)
{
    if (!browser)
        return;

    file_browser_reset_saved_location(&browser->pending_location);
}

FileBrowserActivateResult FileBrowser_Activate(FileBrowser *browser,
                                               char *out_path,
                                               size_t out_path_size,
                                               char *out_member_path,
                                               size_t out_member_path_size)
{
    if (!browser || browser->entries_count <= 0)
        return FILE_BROWSER_ACTIVATE_NONE;

    if (out_path && out_path_size > 0)
        out_path[0] = '\0';
    if (out_member_path && out_member_path_size > 0)
        out_member_path[0] = '\0';

    FileBrowserEntry *entry = &browser->entries[browser->selected_index];
    char path[PATH_MAX] = {0};

    if (entry->type == FILE_BROWSER_ENTRY_PARENT)
    {
        if (browser->mode == FILE_BROWSER_MODE_ZIP)
        {
            FileBrowserZipContext *ctx = file_browser_current_zip(browser);
            if (!ctx)
                return FILE_BROWSER_ACTIVATE_NONE;

            if (ctx->zip_dir[0] != '\0')
            {
                char parent_dir[PATH_MAX] = {0};
                if (!file_browser_parent_zip_dir(ctx->zip_dir, parent_dir, sizeof(parent_dir)))
                    return FILE_BROWSER_ACTIVATE_NONE;
                snprintf(ctx->zip_dir, sizeof(ctx->zip_dir), "%s", parent_dir);

                if (!file_browser_scan_zip(browser))
                    return FILE_BROWSER_ACTIVATE_NONE;

                return FILE_BROWSER_ACTIVATE_NAVIGATED;
            }

            if (ctx->parent)
            {
                file_browser_pop_zip_context(browser);
                if (!file_browser_scan_zip(browser))
                    return FILE_BROWSER_ACTIVATE_NONE;
                return FILE_BROWSER_ACTIVATE_NAVIGATED;
            }

            return file_browser_scan_filesystem(browser, browser->current_dir)
                       ? FILE_BROWSER_ACTIVATE_NAVIGATED
                       : FILE_BROWSER_ACTIVATE_NONE;
        }

        if (!file_browser_parent_directory(browser->current_dir, path, sizeof(path)))
            return FILE_BROWSER_ACTIVATE_NONE;

        if (!file_browser_scan_filesystem(browser, path))
            return FILE_BROWSER_ACTIVATE_NONE;

        return FILE_BROWSER_ACTIVATE_NAVIGATED;
    }

    if (browser->mode == FILE_BROWSER_MODE_ZIP)
    {
        FileBrowserZipContext *ctx = file_browser_current_zip(browser);
        if (!ctx)
            return FILE_BROWSER_ACTIVATE_NONE;

        if (entry->type == FILE_BROWSER_ENTRY_DIRECTORY)
        {
            snprintf(ctx->zip_dir, sizeof(ctx->zip_dir), "%s", entry->path);
            if (!file_browser_scan_zip(browser))
                return FILE_BROWSER_ACTIVATE_NONE;
            return FILE_BROWSER_ACTIVATE_NAVIGATED;
        }

        if (entry->type == FILE_BROWSER_ENTRY_ZIP)
        {
            return file_browser_enter_nested_zip(browser, entry)
                       ? FILE_BROWSER_ACTIVATE_NAVIGATED
                       : FILE_BROWSER_ACTIVATE_NONE;
        }

        if (entry->type != FILE_BROWSER_ENTRY_ZZT)
        {
            FileBrowser_SetStatus(browser, "Only .zzt files can be opened right now.");
            return FILE_BROWSER_ACTIVATE_NONE;
        }

        if (!out_path || out_path_size == 0 || !out_member_path || out_member_path_size == 0)
            return FILE_BROWSER_ACTIVATE_NONE;

        if (snprintf(out_path, out_path_size, "%s", ctx->archive_path) >= (int)out_path_size ||
            snprintf(out_member_path, out_member_path_size, "%s", entry->path) >= (int)out_member_path_size)
        {
            FileBrowser_SetStatus(browser, "Selected zip path is too long.");
            return FILE_BROWSER_ACTIVATE_NONE;
        }

        FileBrowser_ClearStatus(browser);
        return FILE_BROWSER_ACTIVATE_SELECTED_WORLD;
    }

    if (!file_browser_join_path(path, sizeof(path), browser->current_dir, entry->path))
    {
        FileBrowser_SetStatus(browser, "Path is too long.");
        return FILE_BROWSER_ACTIVATE_NONE;
    }

    if (entry->type == FILE_BROWSER_ENTRY_DIRECTORY)
    {
        if (!file_browser_scan_filesystem(browser, path))
            return FILE_BROWSER_ACTIVATE_NONE;

        return FILE_BROWSER_ACTIVATE_NAVIGATED;
    }

    if (entry->type == FILE_BROWSER_ENTRY_ZIP)
    {
        char display_root[FILE_BROWSER_DISPLAY_PATH_MAX] = {0};
        if (snprintf(display_root, sizeof(display_root), "%s", path) >= (int)sizeof(display_root))
        {
            FileBrowser_SetStatus(browser, "Zip path is too long.");
            return FILE_BROWSER_ACTIVATE_NONE;
        }

        if (!file_browser_push_zip_context(browser, path, display_root, false, NULL))
        {
            FileBrowser_SetStatus(browser, "Out of memory while opening zip.");
            return FILE_BROWSER_ACTIVATE_NONE;
        }

        if (!file_browser_scan_zip(browser))
        {
            file_browser_pop_zip_context(browser);
            return FILE_BROWSER_ACTIVATE_NONE;
        }

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
    char display_path[FILE_BROWSER_DISPLAY_PATH_MAX] = {0};

    if (browser->mode == FILE_BROWSER_MODE_ZIP)
    {
        FileBrowserZipContext *ctx = file_browser_current_zip(browser);
        if (!ctx)
            return "";

        if (ctx->zip_dir[0] == '\0')
        {
            file_browser_copy_text(display_path, sizeof(display_path), ctx->display_root);
            file_browser_append_text(display_path, sizeof(display_path), "!/");
        }
        else
        {
            file_browser_copy_text(display_path, sizeof(display_path), ctx->display_root);
            file_browser_append_text(display_path, sizeof(display_path), "!/");
            file_browser_append_text(display_path, sizeof(display_path), ctx->zip_dir);
            file_browser_append_text(display_path, sizeof(display_path), "/");
        }
    }
    else
    {
        file_browser_copy_text(display_path,
                               sizeof(display_path),
                               browser->current_dir[0] ? browser->current_dir : ".");
    }

    size_t len = strlen(display_path);
    if ((int)len <= max_len)
    {
        snprintf(browser->path_buffer,
                 sizeof(browser->path_buffer),
                 "\\f15%s: %.*s",
                 browser->mode == FILE_BROWSER_MODE_ZIP ? "Zip" : "Dir",
                 max_len,
                 display_path);
        return browser->path_buffer;
    }

    const char *tail = display_path + len - (size_t)(max_len - 3);
    snprintf(browser->path_buffer,
             sizeof(browser->path_buffer),
             "\\f15%s: ...%s",
             browser->mode == FILE_BROWSER_MODE_ZIP ? "Zip" : "Dir",
             tail);
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

    return "\\f8Directories are yellow. .zip files are light blue. Nested zips are browsable. Only .zzt files open.";
}
