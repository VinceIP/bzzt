/**
 * @file editor.h
 * @author Vince Patterson (vinceip532@gmail.com)
 * @brief
 * @version 0.1
 * @date 2025-08-09
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <stdbool.h>
#include "engine.h"

struct Engine;
struct Cursor;
struct InputState;

typedef enum EditorState
{
    EDITOR_STATE_MAIN,
    EDITOR_STATE_WAITING_FOR_KEY,
    EDITOR_STATE__COUNT
} EditorState;

typedef struct Editor
{
    EditorState state;
} Editor;

void Editor_Update(Engine *, InputState *);
void Editor_Init(Engine *);
void Editor_Destroy(Engine *);