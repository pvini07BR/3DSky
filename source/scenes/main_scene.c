#include "scenes/main_scene.h"
#include <stdio.h>
#include "clay/clay.h"

#include "bluesky/bluesky.h"

static Scene main_scene;

static void main_init() {
    printf("Hello from main scene\n");
    
}

static void main_update(void) {

}

static void main_layout(void) {
    CLAY_TEXT(CLAY_STRING("Main Scene"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0 }));
    CLAY({
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = {.height = CLAY_SIZING_FIXED(100)},
        },
        .scroll = {
            .horizontal = false,
            .vertical = true,
        },
        .backgroundColor = {0, 0, 0, 255}
    }) {
        for (int i = 0; i < 10; i++) {
            CLAY_TEXT(CLAY_STRING("Hello"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0 }));
        }
    }
}

static void main_unload(void) {
    printf("Bye from main scene\n");
}

Scene* get_main_scene(void) {
    main_scene.init = main_init;
    main_scene.update = main_update;
    main_scene.layout = main_layout;
    main_scene.unload = main_unload;
    
    return &main_scene;
} 