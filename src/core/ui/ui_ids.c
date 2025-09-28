/**
 * @file ui_ids.c
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief Handle allocation of UI unique ids.
 * @version 0.1
 * @date 2025-08-04
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "ui.h"
#include "debugger.h"
#include <stdbool.h>

#define UI_MAX_IDS 4096

static bool id_used[UI_MAX_IDS];
static int next_id = 100;

bool UI_ID_Is_Valid(int id)
{
    return id >= 0 && id < UI_MAX_IDS;
}

bool UI_ID_Register(int id)
{
    if (!UI_ID_Is_Valid(id) || id_used[id])
    {
        return false;
    }
    id_used[id] = true;
    if (id >= next_id)
        next_id = id + 1;
    return true;
}

int UI_ID_Next(void)
{
    while (next_id < UI_MAX_IDS && id_used[next_id])
        next_id++;
    if (next_id >= UI_MAX_IDS)
        return -1;
    id_used[next_id] = true;
    return next_id++;
}