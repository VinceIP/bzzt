/**
 * @file bzzt.h
 * @author your name (you@domain.com)
 * @brief Main library for bzzt engine features
 * @version 0.1
 * @date 2025-08-01
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include "raylib.h"
#include "color.h"
#include "zzt.h"
#define BZZT_BOARD_DEFAULT_W 80
#define BZZT_BOARD_DEFAULT_H 25
#define ZZT_BOARD_DEFAULT_W 60
#define ZZT_BOARD_DEFAULT_H 25
#define BZZT_VIEWPORT_DEFAULT_W 60
#define BZZT_VIEWPORT_DEFAULT_H 25
#define BZZT_MAX_PATH_LENGTH 32

typedef struct InputState InputState;

// The direction an object is facing
typedef enum
{
    DIR_NONE,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// A Bzzt cell.
typedef struct Bzzt_Cell
{
    bool visible;
    uint8_t glyph;
    Color_Bzzt fg, bg;
} Bzzt_Cell;

/**
 * @brief A Bzzt object.
 *
 */
typedef struct Bzzt_Object
{
    int id;         // Unique object id
    int x, y;       // Coordinates in world units
    Direction dir;  // This object's direction.
    Bzzt_Cell cell; // Reference to this object's visual data
} Bzzt_Object;

/**
 * @brief A Bzzt board.
 *
 */
typedef struct Bzzt_Board
{
    int width, height; // Dimensions. Defaults to 80x25

    Bzzt_Object **objects;                        // Dynamic array of all objects on this board.
    int object_count, object_cap, object_next_id; // Array properties

    char *name; // Name of this board

} Bzzt_Board;

typedef struct Bzzt_World
{
    char title[64];
    char author[32];
    char file_path[BZZT_MAX_PATH_LENGTH];
    uint32_t version;

    Bzzt_Board **boards;
    int boards_count;
    int boards_cap;
    int boards_current;

    Bzzt_Object *player;

    bool allow_scroll;
    bool strict_palette;

    bool loaded;
    bool doUnload;
} Bzzt_World;

typedef struct Bzzt_Viewport
{
    Rectangle rect; // x, y, w, h in world units
} Bzzt_Viewport;

typedef struct Bzzt_Camera
{
    Rectangle rect;              // x,y,w,h in world units
    int cell_width, cell_height; // Dimensions of cells in pixels
    Bzzt_Viewport viewport;      // viewport displaying this camera
} Bzzt_Camera;

/* -- Objects --*/
// Create a new Bzzt_Object
Bzzt_Object *Bzzt_Object_Create(uint8_t glyph, Color_Bzzt fg, Color_Bzzt bg, int x, int y);
// Destroy a Bzzt_Object
void Bzzt_Object_Destroy(Bzzt_Object *o);

// Convert a ZZT tile to a Bzzt Object
Bzzt_Object *Bzzt_Object_From_ZZT_Tile(ZZTtile *zztTile, int x, int y);

/* -- --*/

/* -- Boards --*/

// Create a new Bzzt board.
Bzzt_Board *Bzzt_Board_Create(const char *name, int w, int h);

// Destroy a Bzzt board.
void Bzzt_Board_Destroy(Bzzt_Board *b);

// Add an object to a board's object array.
Bzzt_Object *Bzzt_Board_Add_Object(Bzzt_Board *b, Bzzt_Object *o);

// Remove an object from a board by its unique object id.
void Bzzt_Board_Remove_Object(Bzzt_Board *b, int id);

// Return a Bzzt object by its unique object id.
Bzzt_Object *Bzzt_Board_Get_Object(Bzzt_Board *b, int id);

// Convert a ZZT block (decompressed board) to a Bzzt board
Bzzt_Board *Bzzt_Board_From_ZZT_Board(ZZTworld *zw);

/* -- --*/

/* -- World --*/

Bzzt_World *Bzzt_World_Create(char *title);
void Bzzt_World_Add_Board(Bzzt_World *world, Bzzt_Board *board);
int Bzzt_World_Load(Bzzt_World *w, const char *path);
int Bzzt_World_Save(Bzzt_World *w, const char *path);
void Bzzt_World_Unload(Bzzt_World *w);

void Bzzt_World_Update(Bzzt_World *w, InputState *in);

// Convert a ZZT world to a Bzzt world
Bzzt_World *Bzzt_World_From_ZZT_World(char *file);

/* -- --*/

/* -- Camera -- */
Bzzt_Camera *BzztCamera_Create();

/* -- --*/