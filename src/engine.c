#include <stdbool.h>
#include "engine.h"

bool Engine_Init(Engine* e, int w, int h){
    return false;
}

void Engine_Run(Engine* e){
    while(e->running && !WindowShouldClose()){
        //Update input
        //Update world
        //Render
    }
}

void Engine_Quit(Engine* e) {
    //Destroy renderer
    //Destroy world
}