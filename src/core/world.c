#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "raylib.h"
#include "bzzt.h"
#include "input.h"
#include "color.h"
#include "debugger.h"

static void grow_boards_array(Bzzt_World *w)
{
    int new_cap = w->boards_cap * 2;
    Bzzt_Board **tmp = realloc(w->boards, new_cap * sizeof(Bzzt_World *));
    if (!tmp)
        Debug_Printf(LOG_ENGINE, "Error reallocating boards array. Get ready to crash, probably");
    w->boards = tmp;
    w->boards_cap = new_cap;
}

Bzzt_World *Bzzt_World_Create(char *title)
{
    Bzzt_World *w = (Bzzt_World *)malloc(sizeof(Bzzt_World));
    if (!w)
        return NULL;
    strncpy(w->title, title, sizeof(w->title) - 1);
    w->boards_cap = 4;
    w->boards = (Bzzt_Board **)malloc(sizeof(Bzzt_Board *) * w->boards_cap);                      // allocate initial boards size to 4
    w->boards[0] = Bzzt_Board_Create("Title Screen", BZZT_BOARD_DEFAULT_W, BZZT_BOARD_DEFAULT_H); // create a starting empty title screen board

    w->player = Bzzt_Board_Add_Object(w->boards[0], // Pushes a default player obj to the board
                                      Bzzt_Object_Create(2, COLOR_WHITE, COLOR_BLUE, 47, 10));
    w->boards_current = 0;
    w->boards_count = 1;
    w->doUnload = false;
    w->loaded = true;
    return w;
}

void Bzzt_World_Unload(Bzzt_World *w)
{
    if (!w)
        return;
    for (int i = 0; i < w->boards_count; ++i)
    {
        if (w->boards[i])
        {
            Bzzt_Board_Destroy(w->boards[i]);
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

void Bzzt_World_Update(Bzzt_World *w, InputState *in)
{
    if (w->doUnload)
    {
        Bzzt_World_Unload(w);
        return;
    }

    int *dx = &w->player->x;
    int *dy = &w->player->y;
    Bzzt_Board *b = w->boards[w->boards_current];
    Rectangle bounds = {0, 0, b->width, b->height};

    // Handle_Key_Move(dx, dy, bounds, in);
}

void Bzzt_World_Add_Board(Bzzt_World *w, Bzzt_Board *b)
{
    if (w->boards_count >= w->boards_cap)
    {
        grow_boards_array(w);
    }
    w->boards[w->boards_count++] = b;
}

Bzzt_World *Bzzt_World_From_ZZT_World(char *file)
{
    ZZTworld zw = zztWorldLoad(file);
    if (!file)
        Debug_Printf(LOG_ENGINE, "Invalid or unprovided ZZT filename.");
    if (!zw)
        Debug_Printf(LOG_ENGINE, "Error loading ZZT world %s", file);

    Bzzt_World *bw = Bzzt_World_Create(zw.filename);
    bw->file_path = file;
    bw->author = "Blank";

    int boardCount = zw.header->boardcount;

    for (int i = 0; i < boardCount; ++i)
    {
        Bzzt_Board *b;
        ZZTblock *block;
        block = zztBoardGetBlock(zw);
        b = Bzzt_Board_From_ZZT_Board(block);
        Bzzt_World_Add_Board(bw, b);
    }

    zztWorldFree(zw);

    return bw;
}
