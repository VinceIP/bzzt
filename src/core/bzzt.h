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

// tbd -- fill for all zzt types. include bzzt types.
typedef enum
{
    INTERACTION_NONE,
    INTERACTION_KEY,
    INTERACTION_DOOR,
    INTERACTION_GEM,
    INTERACTION_AMMO,
    INTERACTION_BOULDER_PUSH,
    INTERACTION_TORCH,
} Interaction_Type;

// A Bzzt cell.
typedef struct Bzzt_Tile
{
    bool visible, blink;
    uint8_t element;
    uint8_t glyph;
    Color_Bzzt fg, bg;
} Bzzt_Tile;

typedef struct Bzzt_Stat
{
    int x, y;
    int16_t step_x, step_y;
    int16_t cycle;

    uint8_t data[3];
    uint8_t data_label[3];
    int16_t follower, leader;

    Bzzt_Tile under;

    char *program;
    size_t program_length;
    size_t program_counter;
} Bzzt_Stat;

/**
 * @brief A Bzzt object.
 *
 */
typedef struct Bzzt_Object
{
    int id;         // Unique object id
    int x, y;       // Coordinates in world units
    Direction dir;  // This object's direction.
    Bzzt_Tile cell; // Reference to this object's visual data

    uint8_t bzzt_type;
    Bzzt_Stat *param;
    uint8_t under_type;
    uint8_t under_color;

    bool is_bzzt_exclusive; // True if using bzzt-exclusive features
} Bzzt_Object;

/**
 * @brief A Bzzt board.
 *
 */
typedef struct Bzzt_Board
{
    char *name;        // Name of this board
    int width, height; // Dimensions. Defaults to 60x25

    Bzzt_Tile *tiles;

    Bzzt_Stat **stats;
    int stat_count, stat_cap;

    /*for zzt support*/
    uint8_t max_shots;
    uint8_t darkness;
    uint8_t board_n, board_s, board_w, board_e;
    uint8_t reenter;              // Re-enter when zapped
    char message[59];             // board entry message
    uint8_t reenter_x, reenter_y; // Re-enter coordinates
    int16_t time_limit;

    int idx;
} Bzzt_Board;

typedef struct Bzzt_World
{
    char title[64];
    char author[32];
    char file_path[BZZT_MAX_PATH_LENGTH];
    uint32_t version;

    Bzzt_Board **boards;
    int boards_count, boards_cap, boards_current;
    Bzzt_Board *start_board;
    uint16_t start_board_idx;

    bool allow_blink, blink_state;
    int blink_delay_rate; // In ms
    double blink_timer;   // Ms since last blink

    bool paused;
    bool on_title;

    /*for zzt support*/
    int16_t ammo, gems, health, torches, score;
    uint8_t keys[7];
    int16_t torch_cycles, energizer_cycles;
    char flags[10][21];
    int16_t time_passed;
    /*-*/

    bool zzt_compatible; // If this world can be saved as a valid .zzt

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

// Return true if object can be walked on top of.
bool Bzzt_Tile_Is_Walkable(Bzzt_Tile tile);

// Return interaction type of an object.
Interaction_Type Bzzt_Tile_Get_Interaction_Type(Bzzt_Tile tile);

const char *Bzzt_Tile_Get_Type_Name(Bzzt_Tile tile);

Bzzt_Stat *Bzzt_Stat_From_ZZT_Param(ZZTparam *param, ZZTtile tile, int x, int y);

Bzzt_Tile Bzzt_Tile_From_ZZT_Tile(ZZTblock *block, int x, int y);

/* -- --*/

/* -- Boards --*/

// Create a new Bzzt board.
Bzzt_Board *Bzzt_Board_Create(const char *name, int w, int h);

// Destroy a Bzzt board.
void Bzzt_Board_Destroy(Bzzt_Board *b);

// Add an object to a board's object array.
Bzzt_Object *Bzzt_Board_Add_Object(Bzzt_Board *b, Bzzt_Object *o);

// Add a stat to a board.
Bzzt_Stat *Bzzt_Board_Add_Stat(Bzzt_Board *b, Bzzt_Stat *s);

// Remove an object from a board by its unique object id.
void Bzzt_Board_Remove_Stat(Bzzt_Board *b, int id);

Bzzt_Stat *Bzzt_Board_Get_Stat_At(Bzzt_Board *b, int x, int y);

int Bzzt_Board_Get_Stat_Index(Bzzt_Board *b, Bzzt_Stat *stat);

// Return a Bzzt object by its unique object id.
Bzzt_Object *Bzzt_Board_Get_Object(Bzzt_Board *b, int id);

Bzzt_Tile Bzzt_Board_Get_Tile(Bzzt_Board *b, int x, int y);

bool Bzzt_Board_Set_Tile(Bzzt_Board *b, int x, int y, Bzzt_Tile tile);

// Convert the currently selected board in a ZZT world to a Bzzt board
Bzzt_Board *Bzzt_Board_From_ZZT_Board(ZZTworld *zw);

/* -- --*/

/* -- World --*/

Bzzt_World *Bzzt_World_Create(char *title);
void Bzzt_World_Add_Board(Bzzt_World *world, Bzzt_Board *board);
int Bzzt_World_Load(Bzzt_World *w, const char *path);
int Bzzt_World_Save(Bzzt_World *w, const char *path);
void Bzzt_World_Destroy(Bzzt_World *w);
void Bzzt_World_Update(Bzzt_World *w, InputState *in);
bool Bzzt_World_Switch_Board_To(Bzzt_World *w, int board_idx, int x, int y);
void Bzzt_World_Set_Pause(Bzzt_World *w, bool pause);

// Convert a ZZT world to a Bzzt world
Bzzt_World *Bzzt_World_From_ZZT_World(char *file);

/* -- --*/

/* -- Camera -- */
Bzzt_Camera *BzztCamera_Create();

/* -- --*/