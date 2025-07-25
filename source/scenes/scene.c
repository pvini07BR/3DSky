#include <stddef.h>
#include "scenes/scene.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"

static Scene* current_scene = NULL;

void change_scene(Scene* new_scene) {
    if (current_scene != NULL && current_scene->unload != NULL) {
        current_scene->unload();
    }
    
    current_scene = new_scene;

    Clay_Citro2d_ClearTextCacheAndBuffer();
    
    if (current_scene != NULL && current_scene->init != NULL) {
        current_scene->init();
    }
}

Scene* get_current_scene(void) {
    return current_scene;
} 