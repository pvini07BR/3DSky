#ifndef SCENE_H
#define SCENE_H

#include "clay/clay.h"

typedef struct {
    void (*init)(void);
    void (*update)(void);

    void (*layout_top)(void);
    void (*layout_bottom)(void);

    void (*unload)(void);
} Scene;

void change_scene(Scene* new_scene);

Scene* get_current_scene(void);

#endif