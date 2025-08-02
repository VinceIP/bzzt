/**
 * @file bz_char.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-01
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bz_char.h"

static bool bzc_read_header(FILE *fp, BZCHeader *h)
{
    // If whatever we loaded doesn't have a proper bcz header
    if (fread(h, sizeof(BZCHeader), 1, fp) != 1)
        return false;
    if (memcmp(h->magic, "Bzc", 3) != 0)
        return false;

    return true;
}

bool BZC_Load(const char *path, BzztCharset *out)
{
    FILE *fp = fopen(path, "rb");

    if (!fp)
        return false;
    // Check for valid header
    if (!bzc_read_header(fp, &out->header))
    {
        fclose(fp);
        return false;
    }

    size_t bytes =
        out->header.glyph_count *
        out->header.glyph_h *
        out->header.glyph_w *
        out->header.bpp / 8;

    out->pixels = malloc(bytes);

    if (!out->pixels)
    {
        fclose(fp);
        return false;
    }

    fread(out->pixels, bytes, 1, fp);
    fclose(fp);

    return true;
    // Could turn this data into a Raylib Image here
}

bool BZC_Save(const char *path, BzztCharset *in)
{
    FILE *fp = fopen(path, "wb");
    if (!fp)
        return false;

    fwrite(&in->header, sizeof(BZCHeader), 1, fp);
    size_t bytes = in->header.glyph_count *
                   in->header.glyph_h *
                   in->header.glyph_w * in->header.bpp / 8;

    fwrite(in->pixels, bytes, 1, fp);
    fclose(fp);
    return true;
}