#ifndef MAIN_SCENE_H
#define MAIN_SCENE_H

#include "scene.h"

typedef struct {
    char access_token[1024];
    char refresh_token[1024];
} MainSceneData;

Scene* get_main_scene(void);
void set_main_scene_tokens(const char* access_token, const char* refresh_token);
MainSceneData* get_main_scene_data(void);

#endif