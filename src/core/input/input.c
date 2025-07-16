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
    if (a->x == b->x && b->x == b->y)
        return true;
    else
        return false;
}

void Mouse_Poll(MouseState *s)
{
    s->lastScreenPosition = s->screenPosition;
    s->lastWorldPosition = s->worldPosition;

    s->screenPosition = GetMousePosition();
    s->moved = is_vector2_equal(&s->lastScreenPosition, &s->screenPosition);

    s->delta = GetMouseDelta();

    s->leftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    s->leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    s->rightPressed = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    s->rightDown = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    s->middlePressed = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
    s->middleDown = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
    s->wheelMove = GetMouseWheelMove();
}

static Vector2 handle_key_move(int *dx, int *dy, Rectangle bounds, InputState *in)
{
    int newX = *dx + in->dx;
    int newY = *dy + in->dy;

    if (newX >= bounds.x && newX < bounds.width && !in->delayLock)
        *dx = newX;
    if (newY >= bounds.y && newY < bounds.height && !in->delayLock)
        *dy = newY;
    return (Vector2){newX, newY};
}

static Vector2 handle_mouse_move(MouseState *m, BzztCamera *c)
{
    return Camera_ScreenToCell(c, m->screenPosition);
}

Vector2 Handle_Cursor_Move(InputState *in, MouseState *m, BzztCamera *c, Rectangle bounds)
{

    if (in->anyDirPressed)
    {
        if (IsCursorHidden())
            EnableCursor();
        return handle_key_move(&in->dx, &in->dy, bounds, in);
    }
    if (m->moved)
    {
        if (!IsCursorHidden())
            DisableCursor();
        return handle_mouse_move(m, c);
    }
}
