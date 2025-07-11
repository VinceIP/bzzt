#include <stdbool.h>
#include "engine.h"

typedef struct
{
    Cursor *c;
} Editor;
void Editor_Update(Engine *, InputState *);
void Editor_Init(Engine *);