#include <stdbool.h>
#include <stdio.h>
#include "raylib.h"
#include "input.h"
#include "debugger.h"
#include "coords.h"
#include "camera.h"

void Input_Poll(InputState *s)
{
    s->dx = (IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT));
    s->dy = (IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP));
    s->E_pressed = IsKeyPressed(KEY_E);
    s->L_pressed = IsKeyPressed(KEY_L);
    s->Q_pressed = IsKeyPressed(KEY_Q);
    if (s->dx != 0 || s->dy != 0) // If key is down
    {
        s->anyDirPressed = true;
        if (s->heldFrames == 0)
        {
            s->delayLock = false;
        }
        else if (s->heldFrames >= 4)
        {
            s->delayLock = false;
            s->heldFrames = 0;
        }
        else
        {
            s->delayLock = true;
        }
        s->heldFrames++;
    }
    else
    {
        s->anyDirPressed = false;
        s->heldFrames = 0;
        s->delayLock = false;
    }

    s->quit = IsKeyPressed(KEY_ESCAPE) || WindowShouldClose();
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
    if (!in->delayLock)
    {
        int newX = pos.x + in->dx;
        int newY = pos.y + in->dy;

        if (newX >= bounds.x && newX < bounds.x + bounds.width)
            pos.x = newX;
        if (newY >= bounds.y && newY < bounds.y + bounds.height)
            pos.y = newY;
    }
    return pos;
}

static Vector2 handle_mouse_move(MouseState *m, BzztCamera *c, Rectangle bounds, Vector2 currentPos)
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

Vector2 Handle_Cursor_Move(Vector2 currentPos, InputState *in, MouseState *m, BzztCamera *c, Rectangle bounds)
{
    if (in->anyDirPressed) // if arrow keys are pressed
    {
        return handle_key_move(currentPos, bounds, in);
    }
    if (m->moved) // If the mouse has moved
    {
        return handle_mouse_move(m, c, bounds, currentPos);
    }
    return currentPos;
}
