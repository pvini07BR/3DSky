#include "scenes/main_scene.h"
#include <stdio.h>
#include "c2d/spritesheet.h"
#include "clay/clay.h"

#include "bluesky/bluesky.h"
#include "defines.h"

static Scene main_scene;

C2D_SpriteSheet iconsSheet = NULL;
C2D_SpriteSheet otherIconsSheet = NULL;

C2D_Image homeImage;
C2D_Image searchImage;
C2D_Image chatImage;
C2D_Image bellImage;
C2D_Image userImage;

void post_component(Clay_String username, Clay_String handle, Clay_String postText) {
    CLAY({
        .layout = {
            .padding = CLAY_PADDING_ALL(10),
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = 4
        },
        .border = {.width = {.bottom = 1}, .color = {46, 64, 82, 255}}
    }) {
        CLAY_TEXT(CLAY_STRING("pfp"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0 }));
        CLAY({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }) {
            CLAY({
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                    .childGap = 4
                },
            }) {
                CLAY_TEXT(username, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0 }));
                CLAY_TEXT(handle, CLAY_TEXT_CONFIG({ .textColor = {128, 128, 128, 255}, .fontSize = 15, .fontId = 0 }));
            }
            CLAY_TEXT(postText, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0 }));
        }
    }
}

static void main_init() {
    iconsSheet = C2D_SpriteSheetLoad("romfs:/icons.t3x");
    if (!iconsSheet) {
        printf("Failed to load icons sheet\n");
        return;
    }

    otherIconsSheet = C2D_SpriteSheetLoad("romfs:/othericons.t3x");
    if (!otherIconsSheet) {
        printf("Failed to load other icons sheet\n");
        return;
    }

    homeImage = C2D_SpriteSheetGetImage(iconsSheet, 0);
    searchImage = C2D_SpriteSheetGetImage(iconsSheet, 1);
    chatImage = C2D_SpriteSheetGetImage(iconsSheet, 2);
    bellImage = C2D_SpriteSheetGetImage(iconsSheet, 3);
    userImage = C2D_SpriteSheetGetImage(otherIconsSheet, 0);
}

static void main_update(void) {

}

static void main_layout(void) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .padding = { .left = TOP_BOTTOM_DIFF - 1, .right = TOP_BOTTOM_DIFF - 1 }
        },
    }) {
        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = { .top = TOP_HEIGHT }
            },
            .scroll = {
                .horizontal = false,
                .vertical = true,
            },
            .border = {.width = {.left = 1, .right = 1}, .color = {46, 64, 82, 255}}
        }) {
            for (int i = 0; i < 30; i++) {
                post_component(CLAY_STRING("John Doe"), CLAY_STRING("john_doe.bsky.social"), CLAY_STRING("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."));
            }
        }

        Clay_LayoutConfig iconLayout = { .sizing = { .width = CLAY_SIZING_FIXED(32), .height = CLAY_SIZING_FIXED(32) }};
        Clay_Dimensions iconDimensions = { .width = 32, .height = 32 };

        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40)},
                .layoutDirection =CLAY_LEFT_TO_RIGHT,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 32,
                .padding = {.left = 5, .right = 10}
            },
            .border = {.width = {.top= 1 }, .color = {46, 64, 82, 255}},
            .backgroundColor = {22, 30, 39, 255}
        }) {
            CLAY({
                .layout = iconLayout,
                .image = {
                    .imageData = &homeImage,
                    .sourceDimensions = iconDimensions,
                },
                .backgroundColor = {255, 255, 255, 255}
            });
            CLAY({
                .layout = iconLayout,
                .image = {
                    .imageData = &searchImage,
                    .sourceDimensions = iconDimensions,
                },
                .backgroundColor = {255, 255, 255, 255}
            });
            CLAY({
                .layout = iconLayout,
                .image = {
                    .imageData = &chatImage,
                    .sourceDimensions = iconDimensions,
                },
                .backgroundColor = {255, 255, 255, 255}
            });
            CLAY({
                .layout = iconLayout,
                .image = {
                    .imageData = &bellImage,
                    .sourceDimensions = iconDimensions,
                },
                .backgroundColor = {255, 255, 255, 255}
            });

            CLAY({
                .layout = iconLayout,
                .image = {
                    .imageData = &userImage,
                    .sourceDimensions = iconDimensions,
                },
                .backgroundColor = {255, 255, 255, 255}
            });
        }
    }
}

static void main_unload(void) {
    if (iconsSheet) {
        C2D_SpriteSheetFree(iconsSheet);
    }
    if (otherIconsSheet) {
        C2D_SpriteSheetFree(otherIconsSheet);
    }
}

Scene* get_main_scene(void) {
    main_scene.init = main_init;
    main_scene.update = main_update;
    main_scene.layout = main_layout;
    main_scene.unload = main_unload;
    
    return &main_scene;
} 