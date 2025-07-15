/**
 * @file coords.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-07-15
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include "camera.h"
#include "raylib.h"

/**
 * @brief Return true if x,y coords p is inside viewport vp
 *
 * @param vp
 * @param p
 * @return true
 * @return false
 */
static inline bool Viewport_Contains(const Viewport *vp, Vector2 p)
{
    return p.x >= vp->rect.x && p.x < vp->rect.x + vp->rect.width &&
           p.y >= vp->rect.y && p.y < vp->rect.y + vp->rect.height;
}

/**
 * @brief Convert screen pixel coordinates to world units using a camera
 *
 * @param cam
 * @param p
 * @return Vector2
 */
static inline Vector2 Camera_ScreenToCell(const BzztCamera *cam, Vector2 p)
{
    // Get offset
    int relX = (int)p.x - cam->rect.x;
    int relY = (int)p.y - cam->rect.y;

    // return early if p is not in bounds of cam
    if (relX < 0 || relY < 0 || relX >= cam->viewport.rect.width || relY >= cam->viewport.rect.height)
    {
        return (Vector2){-1, -1};
    }

    int cellX = cam->rect.x + (relX / cam->cell_width);
    int cellY = cam->rect.y + (relY / cam->cell_height);
    return (Vector2){cellX, cellY};
}