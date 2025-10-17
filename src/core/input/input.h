/**
 * @file input.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "raylib.h"

#define INITIAL_MOVE_DELAY_MS 200.0
#define KEY_REPEAT_INTERVAL_MS 80.0

typedef struct Bzzt_Camera Bzzt_Camera;
typedef struct Engine Engine;

typedef void (*Key_Handler)(Engine *e);

typedef enum ArrowKey
{
    ARROW_NONE = 0,
    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT
} ArrowKey;

typedef struct InputState
{

    ArrowKey input_buffer[8];
    int input_buffer_count;

    ArrowKey arrow_stack[4];
    int arrow_stack_count;

    double key_repeat_timer_ms;
    bool initial_move_done;

    bool E_pressed;
    bool I_pressed;
    bool L_pressed;
    bool Q_pressed;
    bool P_pressed;
    bool ESC_pressed;
    bool SPACE_pressed;
    bool SHIFT_held;
    bool quit;
    int heldFrames;
    double elapsedTime;
    const int frameDelay;
    bool delayLock;
    Key_Handler key_handler;
} InputState;

typedef struct MouseState
{
    Vector2 screenPosition, worldPosition, delta;
    Vector2 lastScreenPosition, lastWorldPosition;
    bool moved;
    bool leftPressed, rightPressed, middlePressed, leftDown, rightDown, middleDown;
    float wheelMove;
} MouseState;

void Input_Set_Handler(InputState *in, Key_Handler h);
void Input_Poll(InputState *out);
void Mouse_Poll(MouseState *out);
ArrowKey Input_Get_Priority_Direction(InputState *in);
void Input_Get_Direction(ArrowKey key, int *out_dx, int *out_dy);
ArrowKey Input_Pop_Buffered_Direction(InputState *in);
Vector2 Handle_Cursor_Move(Vector2 currentPos, InputState *in, MouseState *m, Bzzt_Camera *c, Rectangle bounds);