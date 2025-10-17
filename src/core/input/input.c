/**
 * @file input.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include "raylib.h"
#include "input.h"
#include "debugger.h"
#include "coords.h"
#include "bzzt.h"

static bool is_key_in_stack(InputState *in, ArrowKey key)
{
    for (int i = 0; i < in->arrow_stack_count; ++i)
    {
        if (in->arrow_stack[i] == key)
            return true;
    }
    return false;
}

static void remove_key_from_stack(InputState *in, ArrowKey key)
{
    int found_idx = -1;
    for (int i = 0; i < in->arrow_stack_count; ++i)
    {
        if (in->arrow_stack[i] == key)
        {
            found_idx = i;
            break;
        }
    }
    if (found_idx >= 0)
    {
        for (int i = found_idx + 1; i < in->arrow_stack_count; ++i)
        {
            in->arrow_stack[i - 1] = in->arrow_stack[i];
        }
        in->arrow_stack_count--;
    }
}

static void push_key_to_front(InputState *in, ArrowKey key)
{
    // Remove if already in stack
    remove_key_from_stack(in, key);

    // Shift all existing keys back
    for (int i = in->arrow_stack_count; i > 0; i--)
    {
        in->arrow_stack[i] = in->arrow_stack[i - 1];
    }

    // Add new key at front
    in->arrow_stack[0] = key;
    if (in->arrow_stack_count < 4)
        in->arrow_stack_count++;
}

ArrowKey Input_Get_Priority_Direction(InputState *in)
{
    if (in->arrow_stack_count > 0)
        return in->arrow_stack[0];
    return ARROW_NONE;
}

void Input_Get_Direction(ArrowKey key, int *out_dx, int *out_dy)
{
    *out_dx = 0;
    *out_dy = 0;
    switch (key)
    {
    case ARROW_UP:
        *out_dy = -1;
        break;
    case ARROW_DOWN:
        *out_dy = 1;
        break;
    case ARROW_LEFT:
        *out_dx = -1;
        break;
    case ARROW_RIGHT:
        *out_dx = 1;
        break;
    case ARROW_NONE:
        break;
    }
}

void Input_Set_Handler(InputState *in, Key_Handler h)
{
    if (!in || !h)
        return;
    in->key_handler = h;
}

void Input_Poll(InputState *in)
{

    bool up_held = IsKeyDown(KEY_UP);
    bool down_held = IsKeyDown(KEY_DOWN);
    bool left_held = IsKeyDown(KEY_LEFT);
    bool right_held = IsKeyDown(KEY_RIGHT);

    bool up_pressed = IsKeyPressed(KEY_UP);
    bool down_pressed = IsKeyPressed(KEY_DOWN);
    bool left_pressed = IsKeyPressed(KEY_LEFT);
    bool right_pressed = IsKeyPressed(KEY_RIGHT);

    if (up_pressed)
        push_key_to_front(in, ARROW_UP);
    if (down_pressed)
        push_key_to_front(in, ARROW_DOWN);
    if (left_pressed)
        push_key_to_front(in, ARROW_LEFT);
    if (right_pressed)
        push_key_to_front(in, ARROW_RIGHT);

    if (!up_held && is_key_in_stack(in, ARROW_UP))
        remove_key_from_stack(in, ARROW_UP);
    if (!down_held && is_key_in_stack(in, ARROW_DOWN))
        remove_key_from_stack(in, ARROW_DOWN);
    if (!left_held && is_key_in_stack(in, ARROW_LEFT))
        remove_key_from_stack(in, ARROW_LEFT);
    if (!right_held && is_key_in_stack(in, ARROW_RIGHT))
        remove_key_from_stack(in, ARROW_RIGHT);

    if (in->arrow_stack_count == 0)
    {
        in->key_repeat_timer_ms = 0.0;
        in->initial_move_done = false;
    }

    in->E_pressed = IsKeyPressed(KEY_E);
    in->L_pressed = IsKeyPressed(KEY_L);
    in->Q_pressed = IsKeyPressed(KEY_Q);
    in->P_pressed = IsKeyPressed(KEY_P);
    in->ESC_pressed = IsKeyPressed(KEY_ESCAPE);
    in->SPACE_pressed = IsKeyPressed(KEY_SPACE);

    in->SHIFT_held = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    in->quit = WindowShouldClose();
}

static bool is_vector2_equal(Vector2 *a, Vector2 *b)
{
    return a->x == b->x && a->y == b->y;
}

void Mouse_Poll(MouseState *s)
{
    s->lastScreenPosition = s->screenPosition;
    s->lastWorldPosition = s->worldPosition;

    s->screenPosition = GetMousePosition();
    s->moved = !is_vector2_equal(&s->lastScreenPosition, &s->screenPosition);
    s->delta = GetMouseDelta();

    s->leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    s->leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    s->rightPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    s->rightDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    s->middlePressed = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
    s->middleDown = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    s->wheelMove = GetMouseWheelMove();
}

static Vector2 handle_key_move(Vector2 pos, Rectangle bounds, InputState *in)
{

}

static Vector2 handle_mouse_move(MouseState *m, Bzzt_Camera *c, Rectangle bounds, Vector2 currentPos)
{
    Vector2 pos = Camera_ScreenToCell(c, m->screenPosition); // Translate screen coords to world coords

    // Determine if the cursor would move out of bounds
    bool invalid = pos.x == -1 || pos.y == -1;
    if (pos.x < bounds.x || pos.x >= bounds.x + bounds.width)
        pos.x = currentPos.x;

    if (pos.y < bounds.y || pos.y >= bounds.y + bounds.height)
        pos.y = currentPos.y;

    if (invalid)
        pos = currentPos;

    m->worldPosition = pos;
    return pos;
}

Vector2 Handle_Cursor_Move(Vector2 currentPos, InputState *in, MouseState *m, Bzzt_Camera *c, Rectangle bounds)
{
}
