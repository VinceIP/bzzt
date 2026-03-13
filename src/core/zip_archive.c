#include "zip_archive.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "miniz.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define ZIP_ARCHIVE_MAX_BYTES (64 * 1024 * 1024)
#define ZIP_ARCHIVE_MAX_ENTRIES 4096
#define ZIP_ARCHIVE_MAX_WORLD_BYTES (8 * 1024 * 1024)
#define ZIP_ARCHIVE_MAX_MEMBER_PATH 512

static void zip_archive_set_error(char *out_error, size_t out_error_size, const char *fmt, ...)
{
    if (!out_error || out_error_size == 0)
        return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(out_error, out_error_size, fmt, args);
    va_end(args);
}

static bool zip_archive_check_file_size(const char *archive_path, char *out_error, size_t out_error_size)
{
    struct stat st = {0};
    if (!archive_path || stat(archive_path, &st) != 0)
    {
        zip_archive_set_error(out_error, out_error_size, "Unable to open that zip archive.");
        return false;
    }

    if ((mz_uint64)st.st_size > ZIP_ARCHIVE_MAX_BYTES)
    {
        zip_archive_set_error(out_error,
                              out_error_size,
                              "Zip archive is too large to inspect safely.");
        return false;
    }

    return true;
}

static bool zip_archive_normalize_path(const char *in_path, char *out_path, size_t out_path_size)
{
    if (!in_path || !out_path || out_path_size == 0)
        return false;

    size_t written = 0;
    bool last_was_slash = false;

    while (*in_path == '/' || *in_path == '\\')
        in_path++;

    while (*in_path)
    {
        char ch = *in_path++;
        if (ch == '\\')
            ch = '/';

        if (ch == '/')
        {
            if (last_was_slash)
                continue;
            last_was_slash = true;
        }
        else
        {
            last_was_slash = false;
        }

        if (written + 1 >= out_path_size)
            return false;

        out_path[written++] = ch;
    }

    while (written > 0 && out_path[written - 1] == '/')
        written--;

    out_path[written] = '\0';

    if (strstr(out_path, "../") || strstr(out_path, "/..") ||
        strcmp(out_path, "..") == 0 || strstr(out_path, "./") ||
        strcmp(out_path, ".") == 0)
    {
        return false;
    }

    return true;
}

static int zip_archive_world_type(const char *path)
{
    const char *ext = path ? strrchr(path, '.') : NULL;
    if (!ext)
        return -1;

    if (strcasecmp(ext, ".zzt") == 0)
        return ZIP_ARCHIVE_ENTRY_ZZT;
    if (strcasecmp(ext, ".bzzt") == 0)
        return ZIP_ARCHIVE_ENTRY_BZZT;

    return -1;
}

static int zip_archive_member_type(const char *path)
{
    const char *ext = path ? strrchr(path, '.') : NULL;
    if (!ext)
        return -1;

    if (strcasecmp(ext, ".zip") == 0)
        return ZIP_ARCHIVE_ENTRY_ZIP;

    return zip_archive_world_type(path);
}

static bool zip_archive_entry_matches_dir(const char *member_path,
                                          const char *current_dir,
                                          char *out_remainder,
                                          size_t out_remainder_size)
{
    if (!member_path || !current_dir || !out_remainder || out_remainder_size == 0)
        return false;

    if (current_dir[0] == '\0')
        return snprintf(out_remainder, out_remainder_size, "%s", member_path) < (int)out_remainder_size;

    size_t dir_len = strlen(current_dir);
    if (strncmp(member_path, current_dir, dir_len) != 0)
        return false;
    if (member_path[dir_len] != '/')
        return false;

    return snprintf(out_remainder, out_remainder_size, "%s", member_path + dir_len + 1) < (int)out_remainder_size;
}

static bool zip_archive_reserve_entries(ZipArchiveEntry **entries, int *cap, int needed)
{
    if (!entries || !cap)
        return false;
    if (*cap >= needed)
        return true;

    int new_cap = *cap == 0 ? 16 : *cap * 2;
    while (new_cap < needed)
        new_cap *= 2;

    ZipArchiveEntry *new_entries = realloc(*entries, sizeof(ZipArchiveEntry) * (size_t)new_cap);
    if (!new_entries)
        return false;

    *entries = new_entries;
    *cap = new_cap;
    return true;
}

static bool zip_archive_has_entry(const ZipArchiveEntry *entries,
                                  int count,
                                  const char *path,
                                  ZipArchiveEntryType type)
{
    if (!entries || !path)
        return false;

    for (int i = 0; i < count; ++i)
    {
        if (entries[i].type == type && strcmp(entries[i].path, path) == 0)
            return true;
    }

    return false;
}

static bool zip_archive_add_entry(ZipArchiveEntry **entries,
                                  int *count,
                                  int *cap,
                                  const char *name,
                                  const char *path,
                                  ZipArchiveEntryType type)
{
    if (!entries || !count || !cap || !name || !path)
        return false;

    if (!zip_archive_reserve_entries(entries, cap, *count + 1))
        return false;

    ZipArchiveEntry *entry = &(*entries)[*count];
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
    (*count)++;
    return true;
}

static int zip_archive_entry_sort_key(ZipArchiveEntryType type)
{
    switch (type)
    {
    case ZIP_ARCHIVE_ENTRY_DIRECTORY:
        return 0;
    case ZIP_ARCHIVE_ENTRY_ZIP:
        return 1;
    case ZIP_ARCHIVE_ENTRY_ZZT:
        return 2;
    case ZIP_ARCHIVE_ENTRY_BZZT:
        return 3;
    default:
        return 4;
    }
}

static int zip_archive_compare_entries(const void *lhs, const void *rhs)
{
    const ZipArchiveEntry *a = (const ZipArchiveEntry *)lhs;
    const ZipArchiveEntry *b = (const ZipArchiveEntry *)rhs;

    int key_a = zip_archive_entry_sort_key(a->type);
    int key_b = zip_archive_entry_sort_key(b->type);
    if (key_a != key_b)
        return key_a - key_b;

    return strcasecmp(a->name, b->name);
}

bool ZipArchive_List(const char *archive_path,
                     const char *current_dir,
                     ZipArchiveEntry **out_entries,
                     int *out_count,
                     char *out_error,
                     size_t out_error_size)
{
    if (!archive_path || !current_dir || !out_entries || !out_count)
        return false;

    *out_entries = NULL;
    *out_count = 0;

    if (!zip_archive_check_file_size(archive_path, out_error, out_error_size))
        return false;

    char normalized_dir[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
    if (!zip_archive_normalize_path(current_dir, normalized_dir, sizeof(normalized_dir)))
    {
        zip_archive_set_error(out_error, out_error_size, "Zip path is too long.");
        return false;
    }

    mz_zip_archive archive;
    MZ_CLEAR_OBJ(archive);
    if (!mz_zip_reader_init_file(&archive, archive_path, 0))
    {
        zip_archive_set_error(out_error, out_error_size, "Unable to read that zip archive.");
        return false;
    }

    ZipArchiveEntry *entries = NULL;
    int count = 0;
    int cap = 0;
    bool found_supported_entry = false;

    mz_uint file_count = mz_zip_reader_get_num_files(&archive);
    if (file_count > ZIP_ARCHIVE_MAX_ENTRIES)
    {
        mz_zip_reader_end(&archive);
        zip_archive_set_error(out_error,
                              out_error_size,
                              "Zip archive has too many entries to inspect safely.");
        return false;
    }

    for (mz_uint i = 0; i < file_count; ++i)
    {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&archive, i, &st))
            continue;

        if (st.m_is_directory || st.m_is_encrypted || !st.m_is_supported)
            continue;

        char member_path[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
        if (!zip_archive_normalize_path(st.m_filename, member_path, sizeof(member_path)))
            continue;

        int type = zip_archive_member_type(member_path);
        if ((int)type < 0)
            continue;

        if ((type == ZIP_ARCHIVE_ENTRY_ZZT || type == ZIP_ARCHIVE_ENTRY_BZZT) &&
            st.m_uncomp_size > ZIP_ARCHIVE_MAX_WORLD_BYTES)
            continue;

        if (type == ZIP_ARCHIVE_ENTRY_ZIP && st.m_uncomp_size > ZIP_ARCHIVE_MAX_BYTES)
            continue;

        found_supported_entry = true;

        char remainder[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
        if (!zip_archive_entry_matches_dir(member_path,
                                           normalized_dir,
                                           remainder,
                                           sizeof(remainder)))
        {
            continue;
        }

        char *slash = strchr(remainder, '/');
        if (slash)
        {
            size_t name_len = (size_t)(slash - remainder);
            if (name_len == 0 || name_len >= ZIP_ARCHIVE_MAX_MEMBER_PATH)
                continue;

            char dir_name[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
            char dir_path[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
            memcpy(dir_name, remainder, name_len);
            dir_name[name_len] = '\0';

            if (normalized_dir[0] == '\0')
                snprintf(dir_path, sizeof(dir_path), "%s", dir_name);
            else
                snprintf(dir_path, sizeof(dir_path), "%s/%s", normalized_dir, dir_name);

            if (zip_archive_has_entry(entries, count, dir_path, ZIP_ARCHIVE_ENTRY_DIRECTORY))
                continue;

            if (!zip_archive_add_entry(&entries, &count, &cap, dir_name, dir_path, ZIP_ARCHIVE_ENTRY_DIRECTORY))
            {
                mz_zip_reader_end(&archive);
                ZipArchive_FreeEntries(entries, count);
                zip_archive_set_error(out_error, out_error_size, "Out of memory while reading zip archive.");
                return false;
            }
            continue;
        }

        if (!zip_archive_add_entry(&entries,
                                   &count,
                                   &cap,
                                   remainder,
                                   member_path,
                                   (ZipArchiveEntryType)type))
        {
            mz_zip_reader_end(&archive);
            ZipArchive_FreeEntries(entries, count);
            zip_archive_set_error(out_error, out_error_size, "Out of memory while reading zip archive.");
            return false;
        }
    }

    mz_zip_reader_end(&archive);

    if (!found_supported_entry)
    {
        ZipArchive_FreeEntries(entries, count);
        zip_archive_set_error(out_error, out_error_size, "Zip contains no .zzt, .bzzt, or .zip files.");
        return false;
    }

    if (count > 1)
    {
        qsort(entries, (size_t)count, sizeof(ZipArchiveEntry), zip_archive_compare_entries);
    }

    *out_entries = entries;
    *out_count = count;
    if (out_error && out_error_size > 0)
        out_error[0] = '\0';
    return true;
}

void ZipArchive_FreeEntries(ZipArchiveEntry *entries, int count)
{
    if (!entries)
        return;

    for (int i = 0; i < count; ++i)
    {
        free(entries[i].name);
        free(entries[i].path);
    }

    free(entries);
}

bool ZipArchive_Extract_World_To_Stream(const char *archive_path,
                                        const char *member_path,
                                        FILE **out_stream,
                                        char *out_error,
                                        size_t out_error_size)
{
    if (!archive_path || !member_path || !out_stream)
        return false;

    *out_stream = NULL;

    if (!zip_archive_check_file_size(archive_path, out_error, out_error_size))
        return false;

    char normalized_member[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
    if (!zip_archive_normalize_path(member_path, normalized_member, sizeof(normalized_member)))
    {
        zip_archive_set_error(out_error, out_error_size, "Zip member path is too long.");
        return false;
    }

    mz_zip_archive archive;
    MZ_CLEAR_OBJ(archive);
    if (!mz_zip_reader_init_file(&archive, archive_path, 0))
    {
        zip_archive_set_error(out_error, out_error_size, "Unable to read that zip archive.");
        return false;
    }

    FILE *stream = NULL;
    bool extracted = false;
    mz_uint file_count = mz_zip_reader_get_num_files(&archive);
    if (file_count > ZIP_ARCHIVE_MAX_ENTRIES)
    {
        mz_zip_reader_end(&archive);
        zip_archive_set_error(out_error,
                              out_error_size,
                              "Zip archive has too many entries to inspect safely.");
        return false;
    }

    for (mz_uint i = 0; i < file_count; ++i)
    {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&archive, i, &st))
            continue;

        char candidate_path[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
        if (!zip_archive_normalize_path(st.m_filename, candidate_path, sizeof(candidate_path)))
            continue;

        if (strcmp(candidate_path, normalized_member) != 0)
            continue;

        int type = zip_archive_world_type(candidate_path);
        if ((int)type < 0 || st.m_is_directory || st.m_is_encrypted || !st.m_is_supported)
            break;

        if (st.m_uncomp_size > ZIP_ARCHIVE_MAX_WORLD_BYTES)
        {
            zip_archive_set_error(out_error,
                                  out_error_size,
                                  "Selected world inside zip is too large.");
            break;
        }

        stream = tmpfile();
        if (!stream)
        {
            zip_archive_set_error(out_error, out_error_size, "Unable to create a temporary world file.");
            break;
        }

        if (!mz_zip_reader_extract_to_cfile(&archive, i, stream, 0))
        {
            zip_archive_set_error(out_error, out_error_size, "Failed to extract selected world from zip.");
            fclose(stream);
            stream = NULL;
            break;
        }

        rewind(stream);
        *out_stream = stream;
        extracted = true;
        break;
    }

    mz_zip_reader_end(&archive);

    if (!extracted)
    {
        if (stream)
            fclose(stream);
        if (out_error && out_error[0] == '\0')
            zip_archive_set_error(out_error, out_error_size, "Unable to extract selected world from zip.");
    }

    return extracted;
}

bool ZipArchive_Extract_Member_To_Temp_File(const char *archive_path,
                                            const char *member_path,
                                            char *out_temp_path,
                                            size_t out_temp_path_size,
                                            char *out_error,
                                            size_t out_error_size)
{
    if (!archive_path || !member_path || !out_temp_path || out_temp_path_size == 0)
        return false;

    out_temp_path[0] = '\0';

    if (!zip_archive_check_file_size(archive_path, out_error, out_error_size))
        return false;

    char normalized_member[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
    if (!zip_archive_normalize_path(member_path, normalized_member, sizeof(normalized_member)))
    {
        zip_archive_set_error(out_error, out_error_size, "Zip member path is too long.");
        return false;
    }

    mz_zip_archive archive;
    MZ_CLEAR_OBJ(archive);
    if (!mz_zip_reader_init_file(&archive, archive_path, 0))
    {
        zip_archive_set_error(out_error, out_error_size, "Unable to read that zip archive.");
        return false;
    }

    bool extracted = false;
    mz_uint file_count = mz_zip_reader_get_num_files(&archive);
    if (file_count > ZIP_ARCHIVE_MAX_ENTRIES)
    {
        mz_zip_reader_end(&archive);
        zip_archive_set_error(out_error,
                              out_error_size,
                              "Zip archive has too many entries to inspect safely.");
        return false;
    }

    for (mz_uint i = 0; i < file_count; ++i)
    {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&archive, i, &st))
            continue;

        char candidate_path[ZIP_ARCHIVE_MAX_MEMBER_PATH] = {0};
        if (!zip_archive_normalize_path(st.m_filename, candidate_path, sizeof(candidate_path)))
            continue;

        if (strcmp(candidate_path, normalized_member) != 0)
            continue;

        int type = zip_archive_member_type(candidate_path);
        if (type != ZIP_ARCHIVE_ENTRY_ZIP || st.m_is_directory || st.m_is_encrypted || !st.m_is_supported)
            break;

        if (st.m_uncomp_size > ZIP_ARCHIVE_MAX_BYTES)
        {
            zip_archive_set_error(out_error,
                                  out_error_size,
                                  "Selected zip inside archive is too large.");
            break;
        }

        char temp_template[] = "/tmp/bzzt-zip-XXXXXX";
        int fd = mkstemp(temp_template);
        if (fd < 0)
        {
            zip_archive_set_error(out_error, out_error_size, "Unable to create a temporary zip file.");
            break;
        }

        FILE *stream = fdopen(fd, "w+b");
        if (!stream)
        {
            close(fd);
            unlink(temp_template);
            zip_archive_set_error(out_error, out_error_size, "Unable to create a temporary zip file.");
            break;
        }

        if (!mz_zip_reader_extract_to_cfile(&archive, i, stream, 0))
        {
            fclose(stream);
            unlink(temp_template);
            zip_archive_set_error(out_error, out_error_size, "Failed to extract selected zip from archive.");
            break;
        }

        fclose(stream);
        if (snprintf(out_temp_path, out_temp_path_size, "%s", temp_template) >= (int)out_temp_path_size)
        {
            unlink(temp_template);
            zip_archive_set_error(out_error, out_error_size, "Temporary zip path is too long.");
            break;
        }

        extracted = true;
        break;
    }

    mz_zip_reader_end(&archive);

    if (!extracted && out_error && out_error[0] == '\0')
        zip_archive_set_error(out_error, out_error_size, "Unable to extract selected zip from archive.");

    return extracted;
}
