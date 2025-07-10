#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"
#include "world.h"
#include "object.h"
#include "input.h"
#include "color.h"

World *world_create(char *title)
{
    World *w = (World *)malloc(sizeof(World));
    if (!w)
        return NULL;
    strncpy(w->title, title, sizeof(w->title) - 1);
    w->boards_cap = 4;
    w->boards = (Board **)malloc(sizeof(Board *) * w->boards_cap);                 // allocate initial boards size to 4
    w->boards[0] = board_create("Title Screen", BOARD_DEFAULT_W, BOARD_DEFAULT_H); // create a starting empty title screen board

    w->player = board_add_obj(w->boards[0], // Pushes a default player obj to the board
                              object_create(2, COLOR_WHITE, COLOR_BLUE, 40, 14));
    w->boards_current = 0;
    w->boards_count = 1;
    w->doUnload = false;
    w->loaded = true;
    return w;
}

void World_Unload(World *w)
{
    if (!w)
        return;
    for (int i = 0; i < w->boards_count; ++i)
    {
        if (w->boards[i])
        {
            board_destroy(w->boards[i]);
        }
    }
    free(w->boards);
    free(w);
}

static void handle_input(World *w, InputState *in)
{
    Board *b = w->boards[w->boards_current];
    Rectangle bounds = {
        0, 0, b->width, b->height};
    Handle_Key_Move(&w->player->x, &w->player->y, bounds, in);
}

void World_Update(World *w, InputState *in)
{
    if (w->doUnload)
    {
        World_Unload(w);
        return;
    }
    handle_input(w, in);
}
