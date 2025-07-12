#pragma once
#include "board.h"
#include "input.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_PATH 32

struct World;
struct Object;

typedef struct World
{
    char title[64];
    char author[32];
    char file_path[MAX_PATH];
    uint32_t version;

    Board **boards;
    int boards_count;
    int boards_cap;
    int boards_current;

    Object *player;

    bool allow_scroll;
    bool strict_palette;

    bool loaded;
    bool doUnload;
} World;

World *world_create(char *title);

int world_load(World *w, const char *path);
int world_save(World *w, const char *path);
void World_Unload(World *w);

void World_Update(World *w, InputState *in);