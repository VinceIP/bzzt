#include <stdbool.h>

struct Engine;
struct Cursor;
struct Input;

typedef struct
{
    Cursor *c;
} Editor;
void Editor_Update(Engine *, InputState *);
void Editor_Init(Engine *);