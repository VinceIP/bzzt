#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct FileBrowser FileBrowser;

typedef enum FileBrowserActivateResult
{
    FILE_BROWSER_ACTIVATE_NONE = 0,
    FILE_BROWSER_ACTIVATE_NAVIGATED,
    FILE_BROWSER_ACTIVATE_SELECTED_WORLD
} FileBrowserActivateResult;

FileBrowser *FileBrowser_Create(void);
void FileBrowser_Destroy(FileBrowser *browser);

bool FileBrowser_Open(FileBrowser *browser, const char *start_dir);
bool FileBrowser_MoveSelection(FileBrowser *browser, int delta);
bool FileBrowser_MovePage(FileBrowser *browser, int page_delta);
void FileBrowser_SetVisibleRows(FileBrowser *browser, int rows);
void FileBrowser_SetStatus(FileBrowser *browser, const char *status);
void FileBrowser_ClearStatus(FileBrowser *browser);

FileBrowserActivateResult FileBrowser_Activate(FileBrowser *browser,
                                               char *out_path,
                                               size_t out_path_size);

const char *FileBrowser_FormatDirectory(void *ud);
const char *FileBrowser_FormatEntries(void *ud);
const char *FileBrowser_FormatStatus(void *ud);
