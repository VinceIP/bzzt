
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "object.h"

#define START_CAP 64

Board *Board_Create(const char *name, int w, int h)
{
    Board *b = malloc(sizeof(Board));
    if (!b)
        return NULL;
    b->width = w;
    b->height = h;
    b->object_cap = START_CAP;
    b->objects = malloc(sizeof(Object *) * START_CAP);
    b->name = strdup(name ? name : "Untitled");
    b->object_count = 0;
    b->object_next_id = 1;
    return b;
}

/**
 * @brief Expand board size in memory if needed.
 *
 * @param b Target board.
 */
static int board_grow(Board *b)
{
    int new_cap = b->object_cap * 2;
    Object **tmp = realloc(b->objects, new_cap * sizeof(Object *));
    if (!tmp)
        return -1;

    b->objects = tmp;
    b->object_cap = new_cap;
    return 0;
}

void Board_Destroy(Board *b)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        Object_Destroy(b->objects[i]);
    }
    free(b->objects);
    free(b->name);
    free(b);
}

Object *Board_Add_Obj(Board *b, Object *o)
{
    if (b->object_count == b->object_cap && board_grow(b) != 0)
        return NULL;

    o->id = b->object_next_id++;
    b->objects[b->object_count++] = o;
    return o;
}

void Board_Remove_Obj(Board *b, int id)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        if (b->objects[i]->id == id)
        {
            Object_Destroy(b->objects[i]);
            b->objects[i] = b->objects[--b->object_count];
            return;
        }
    }
}

Object *Board_Get_Obj(Board *b, int id)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        if (b->objects[i]->id == id)
        {
            return b->objects[i];
        }
    }
    return NULL;
}
