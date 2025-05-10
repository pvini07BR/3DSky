#include <stdio.h>
#include <jansson.h>
#include <citro2d.h>
#include "clay/clay.h"
#include "bluesky/bluesky.h"

#include "scenes/main_scene.h"
#include "pages/timeline.h"

#include "defines.h"

static Scene main_scene;

C2D_SpriteSheet iconsSheet = NULL;
C2D_SpriteSheet otherIconsSheet = NULL;

C2D_Image homeImage;
C2D_Image searchImage;
C2D_Image chatImage;
C2D_Image bellImage;
C2D_Image userImage;

TimelinePage timelineData = {
    .cursor = NULL,
    .posts = {},
    .postsLoaded = false
};

// TODO: Add more pages (notifications, chat, profile, etc.)
static void main_init() {
    iconsSheet = C2D_SpriteSheetLoad("romfs:/icons.t3x");
    if (iconsSheet) {
        homeImage = C2D_SpriteSheetGetImage(iconsSheet, 0);
        searchImage = C2D_SpriteSheetGetImage(iconsSheet, 1);
        chatImage = C2D_SpriteSheetGetImage(iconsSheet, 2);
        bellImage = C2D_SpriteSheetGetImage(iconsSheet, 3);
    }

    otherIconsSheet = C2D_SpriteSheetLoad("romfs:/othericons.t3x");
    if (otherIconsSheet) {
        userImage = C2D_SpriteSheetGetImage(otherIconsSheet, 0);
    }

    timeline_page_load_posts(&timelineData);

    // I was testing saving downloaded images to the SD card
    /*
    testImg = download_image_from_url("https://assets.mcasset.cloud/1.21.5/assets/minecraft/textures/item/diamond.png");    
    FILE *fptr = fopen("/3ds/3dsky/cache/testImg.bin", "wb");
    if (fptr == NULL) {
        perror("Failed to open file for writing");
        fclose(fptr);
        return;
    }

    fwrite(testImg.tex->data, 1, testImg.tex->width * testImg.tex->height * 4, fptr);
    fclose(fptr);
    */
}

static void main_layout(void) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .padding = { .left = TOP_BOTTOM_DIFF - 1, .right = TOP_BOTTOM_DIFF - 1 }
        },
    }) {
        timeline_page_layout(&timelineData);
        
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

static void main_update(void) {
    
}

static void main_unload(void) {
    timeline_free_data(&timelineData);

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
