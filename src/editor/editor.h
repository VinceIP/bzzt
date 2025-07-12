#include <stdbool.h>
#include "engine.h"

struct Engine;
struct Cursor;
struct InputState;

typedef struct
{
    Cursor *c;
} Editor;
void Editor_Update(Engine *, InputState *);
void Editor_Init(Engine *);