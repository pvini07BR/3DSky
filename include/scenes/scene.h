#ifndef SCENE_H
#define SCENE_H

#include "thirdparty/clay/clay.h"

typedef struct {
    void (*init)(void);
    void (*update)(float deltaTime);

    void (*layout)(void);

    void (*unload)(void);
} Scene;

void change_scene(Scene* new_scene);

Scene* get_current_scene(void);

#endif