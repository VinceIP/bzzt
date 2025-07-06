#include <stdlib.h>
#include "world.h"
#include "object.h"
#include "color.h"

World *world_create(char *title)
{
    World w = {0};
    w.boards = (Board **)malloc(sizeof(Board *) * 4);                 // allocate initial boards size to 4
    w.boards[0] = ("Title Screen", BOARD_DEFAULT_W, BOARD_DEFAULT_H); // create a starting empty title screen board

    board_add_obj(w.boards[0],                                        // Pushes a default player obj to the board
                  *object_create(2, COLOR_WHITE, COLOR_BLUE, 40, 14));
    w.boards_current = 0;
    return &w;
}