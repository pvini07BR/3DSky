#ifndef MAIN_SCENE_H
#define MAIN_SCENE_H

#include "scene.h"

typedef enum {
    HOME = 0,
    PROFILE = 1,
    //SEARCH = 1,
    //CHAT = 2,
    //NOTIFICATIONS = 3,
    //PROFILE = 4
} Pages;

void main_scene_change_to_profile(const char* handle);
void main_scene_change_page(Pages page);
Scene* get_main_scene(void);

#endif