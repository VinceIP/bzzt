#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"
#include "world.h"
#include "object.h"
#include "input.h"
#include "color.h"

World *World_Create(char *title)
{
    World *w = (World *)malloc(sizeof(World));
    if (!w)
        return NULL;
    strncpy(w->title, title, sizeof(w->title) - 1);
    w->boards_cap = 4;
    w->boards = (Board **)malloc(sizeof(Board *) * w->boards_cap);                 // allocate initial boards size to 4
    w->boards[0] = Board_Create("Title Screen", BOARD_DEFAULT_W, BOARD_DEFAULT_H); // create a starting empty title screen board

    w->player = Board_Add_Obj(w->boards[0], // Pushes a default player obj to the board
                              Object_Create(2, COLOR_WHITE, COLOR_BLUE, 47, 10));
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
            Board_Destroy(w->boards[i]);
            w->boards[i] = NULL;
        }
    }
    w->boards_count = 0;
    w->boards_current = 0;
    w->doUnload = false;
    w->loaded = false;
    w->player = NULL;
    free(w->boards);
}

void World_Update(World *w, InputState *in)
{
    if (w->doUnload)
    {
        World_Unload(w);
        return;
    }

    int *dx = &w->player->x;
    int *dy = &w->player->y;
    Board *b = w->boards[w->boards_current];
    Rectangle bounds = {0, 0, b->width, b->height};

    Handle_Key_Move(dx, dy, bounds, in);
}
