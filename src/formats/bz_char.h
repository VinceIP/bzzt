/**
 * @file bz_char.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief Defines the .bzc (bzzt character set) file format for encoding custom glyphs
 * @version 0.1
 * @date 2025-08-01
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdint.h>
#include <stdbool.h>

#pragma pack(push, 1)
typedef struct BZCHeader
{
    char magic[4];                // "Bzc\0"
    uint8_t ver_major, ver_minor; // bzzt engine version this set is for
    uint8_t flags;                // probably unused for now
    uint8_t bpp;                  // bits per pixel - probably just 1 for the foreseeable future
    uint16_t glyph_w, glyph_h;    // Dimensions of each glyph
    uint16_t glyph_count;
    uint16_t reserved;
} BZCHeader;
#pragma pack(pop)

#pragma once

typedef struct BzztCharset
{
    BZCHeader header;
    uint8_t *pixels;
} BzztCharset;

bool BZC_Load(const char *path, BzztCharset *out);
bool BZC_Save(const char *path, BzztCharset *in);