#pragma once
#include "object.h"

#define BOARD_DEFAULT_W 80
#define BOARD_DEFAULT_H 25

typedef struct
{
    int width, height;

    Object **objects;
    int object_count, object_cap, object_next_id;

    char *name;

} Board;

/**
 * @brief Create a new board.
 * 
 * @param name 
 * @param w 
 * @param h 
 * @return Board 
 */
Board *Board_Create(const char *name, int w, int h);

/**
 * @brief Destroy the given board.
 * 
 * @param b 
 */
void Board_Destroy(Board *b);

/**
 * @brief Add an object to a board.
 *
 * @param b Target board.
 * @param proto The object.
 * @return Object* Pointer to new object.
 */
Object *Board_Add_Obj(Board *b, Object *o);
/**
 * @brief Remove an object from a board by id.
 *
 * @param b Target board.
 * @param id Target object id.
 */
void Board_Remove_Obj(Board *b, int id);
/**
 * @brief Return an object by its id.
 *
 * @param b Target board.
 * @param id Target object id.
 * @return Object* Target object.
 */
Object *Board_Get_Obj(Board *b, int id);

