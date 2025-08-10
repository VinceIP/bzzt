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

typedef struct
{
    Cursor *c;
    UIElement_Text *prompts;
    int prompt_count;
} Editor;

Editor *Editor_Create(Engine *e);
void Editor_Destroy(Editor *editor);
void Editor_Update(Engine *, InputState *);
void Editor_Init(Engine *);