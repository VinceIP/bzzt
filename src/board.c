#include "board.h"
#include <stdlib.h>
#include <string.h>

#define START_CAP 64

Board *board_create(const char *name, int w, int h)
{
    Board *b = malloc(sizeof(Board));
    if (!b)
        return NULL;
    b->width = w;
    b->height = h;
    b->object_cap = START_CAP;
    b->objects = malloc(sizeof(Object*) * START_CAP);
    b->name = strdup(name ? name : "Untitled");
    b->object_count = 0;
    b->object_next_id = 1;
    return b;
}

void board_destroy(Board *b)
{
    for(int i = 0; i <  b->object_count; ++i){
        free(b->objects[i]);
    }
    free(b->objects);
    free(b->name);
    free(b);
}

/**
 * @brief Expand board size in memory if needed.
 *
 * @param b Target board.
 */
static void board_grow(Board *b)
{
    b->object_cap *= 2;
    b->objects = realloc(b->objects, b->object_cap * sizeof(Object *));
    if (!b->objects)
        return;
}

Object *board_add_obj(Board *b, Object *o)
{
    if (b->object_count == b->object_cap)
        board_grow(b);

    o->id = b->object_next_id++;
    b->objects[b->object_count++] = o;
    return o;
}

void board_remove_obj(Board *b, int id)
{
    for (int i = 0; i < b->object_count; ++i)
    {
        if (b->objects[i]->id == id)
        {
            b->objects[i] = b->objects[--b->object_count];
            return;
        }
    }
}

Object *board_get_obj(Board *b, int id)
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
