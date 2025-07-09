#include <stdbool.h>
#include "engine.h"

typedef enum{
    PLAY_MODE,
    EDIT_MODE
} EState;

typedef struct{
    EState state;
} Engine;

bool Engine_Init(Engine* e, int w, int h){
    return false;
}

void Engine_Run(Engine* e){
        //Update input
        //Update world
        //Render
}

void Engine_Quit(Engine* e) {
    //Destroy renderer
    //Destroy world
}