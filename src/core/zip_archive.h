#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum ZipArchiveEntryType
{
    ZIP_ARCHIVE_ENTRY_DIRECTORY = 0,
    ZIP_ARCHIVE_ENTRY_ZIP,
    ZIP_ARCHIVE_ENTRY_ZZT,
    ZIP_ARCHIVE_ENTRY_BZZT
} ZipArchiveEntryType;

typedef struct ZipArchiveEntry
{
    char *name;
    char *path;
    ZipArchiveEntryType type;
} ZipArchiveEntry;

bool ZipArchive_List(const char *archive_path,
                     const char *current_dir,
                     ZipArchiveEntry **out_entries,
                     int *out_count,
                     char *out_error,
                     size_t out_error_size);
void ZipArchive_FreeEntries(ZipArchiveEntry *entries, int count);

bool ZipArchive_Extract_World_To_Stream(const char *archive_path,
                                        const char *member_path,
                                        FILE **out_stream,
                                        char *out_error,
                                        size_t out_error_size);
bool ZipArchive_Extract_Member_To_Temp_File(const char *archive_path,
                                            const char *member_path,
                                            char *out_temp_path,
                                            size_t out_temp_path_size,
                                            char *out_error,
                                            size_t out_error_size);
