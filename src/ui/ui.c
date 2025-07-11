#include <stdio.h>
#include <stdlib.h>
#include "ui.h"
#include "cJSON.h"

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror(path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char *buf = malloc(len + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

PlaysciiAsset *Load_Playscii(const char *path)
{
    printf("Loading playscii from: %s\n", path);

    char *text = read_file(path);
    if (!text)
        return NULL;

    cJSON *json = cJSON_Parse(text);
    if(!json){
        fprintf(stderr, "JSON error before: %s\n", cJSON_GetErrorPtr());
        free(text);
        return NULL;
    }

    char *pretty = cJSON_Print(json);
    puts(pretty);
    free(pretty);
    free(text);

    cJSON_Delete(json);
    return NULL;
}

void Unload_Playscii(PlaysciiAsset *asset, cJSON *json)
{
    free(asset);
}