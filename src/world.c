#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "object.h"
#include "color.h"

World *world_create(char *title)
{
    World *w = (World *)malloc(sizeof(World));
    if (!w)
        return NULL;
    strcpy(w->title, title);
    w->boards_cap = 4;
    w->boards = (Board **)malloc(sizeof(Board *) * w->boards_cap);                 // allocate initial boards size to 4
    w->boards[0] = board_create("Title Screen", BOARD_DEFAULT_W, BOARD_DEFAULT_H); // create a starting empty title screen board

    w->player = board_add_obj(w->boards[0], // Pushes a default player obj to the board
                              object_create(3, COLOR_LIGHT_GREEN, COLOR_BLUE, 40, 14));
    w->boards_current = 0;
    w->boards_count = 1;
    return w;
}

void world_unload(World *w)
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