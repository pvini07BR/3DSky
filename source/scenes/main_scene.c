#include "scenes/main_scene.h"
#include <stdio.h>
#include "clay/clay.h"
#include <string.h>

static Scene main_scene;
static MainSceneData main_data;

void set_main_scene_tokens(const char* access_token, const char* refresh_token) {
    strncpy(main_data.access_token, access_token, sizeof(main_data.access_token) - 1);
    strncpy(main_data.refresh_token, refresh_token, sizeof(main_data.refresh_token) - 1);
    main_data.access_token[sizeof(main_data.access_token) - 1] = '\0';
    main_data.refresh_token[sizeof(main_data.refresh_token) - 1] = '\0';
}

MainSceneData* get_main_scene_data(void) {
    return &main_data;
}

static void main_init() {
    printf("Hello from main scene\n");
    printf("Access g Token: %s\n", main_data.access_token);
    printf("Refresh g Token: %s\n", main_data.refresh_token);
}

static void main_update(void) {
}

static void main_layout_top(void) {
    
    CLAY_TEXT(CLAY_STRING("Main Scene"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0 }));
}

static void main_layout_bottom(void) {
    CLAY_TEXT(CLAY_STRING("Main Scene"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0 }));
}

static void main_unload(void) {
    printf("Bye from main scene\n");
}

Scene* get_main_scene(void) {
    main_scene.init = main_init;
    main_scene.update = main_update;
    main_scene.layout_top = main_layout_top;
    main_scene.layout_bottom = main_layout_bottom;
    main_scene.unload = main_unload;
    
    return &main_scene;
} 