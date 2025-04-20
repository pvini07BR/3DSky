#include <stdio.h>
#include <jansson.h>
#include <citro2d.h>
#include "3ds/thread.h"
#include "clay/clay.h"
#include "bluesky/bluesky.h"

#include "scenes/main_scene.h"
#include "components/post.h"

#include "defines.h"

static Scene main_scene;

C2D_SpriteSheet iconsSheet = NULL;
C2D_SpriteSheet otherIconsSheet = NULL;

C2D_Image homeImage;
C2D_Image searchImage;
C2D_Image chatImage;
C2D_Image bellImage;
C2D_Image userImage;

const char* cursor = NULL;
struct Post posts[50];
bool postsLoaded = false;

Thread thread;
Handle threadReq;
bool runPostLoadingThread = true;

void postLoadingThread(void* arg) {
    while (runPostLoadingThread) {
        svcWaitSynchronization(threadReq, U64_MAX);
		svcClearEvent(threadReq);

        bs_client_pagination_opts opts = {
            .cursor = cursor,
            .limit = 50
        };
        bs_client_response_t* response = bs_client_timeline_get(&opts);
        if (response->err_msg == NULL && response->err_code == 0) {
            json_error_t error;
            json_t* root = json_loads(response->resp, 0, &error);
            if (!root) {
                fprintf(stderr, "Error parsing string at line %d: %s\n", error.line, error.text);
                return;
            }

            json_t* cursor_json = json_object_get(root, "cursor");
            if (cursor_json != NULL) {
                cursor = json_string_value(cursor_json);
            }

            json_t *posts_array = json_object_get(root, "feed");
            if (!json_is_array(posts_array)) { 
                printf("Feed is not an array\n");
                return;
            }

            for (size_t i = 0; i < json_array_size(posts_array); i++) {
                if (i >= 50) { break; }

                json_t* post = json_array_get(posts_array, i);
                
                json_t* post_data = json_object_get(post, "post");
                json_t* author = json_object_get(post_data, "author");

                posts[i].displayName = json_string_value(json_object_get(author, "displayName"));
                posts[i].handle = json_string_value(json_object_get(author, "handle"));

                json_t* record = json_object_get(post_data, "record");

                posts[i].postText = json_string_value(json_object_get(record, "text"));
            }
        }
        bs_client_response_free(response);
        postsLoaded = true;
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

    svcCreateEvent(&threadReq,0);
    thread = threadCreate(postLoadingThread, 0, (16 * 1024), 0x3f, -2, true);
    svcSignalEvent(threadReq);
}

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (postsLoaded) {
        if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
            postsLoaded = false;
            svcSignalEvent(threadReq);
        }
    }
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
                .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH+2), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = { .top = TOP_HEIGHT },
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER}
            },
            .scroll = {
                .horizontal = false,
                .vertical = true,
            },
            .border = {
                .width = {
                    .left = 1,
                    .right = 1,
                    .betweenChildren = CLAY_TOP_TO_BOTTOM
                },
                .color = {46, 64, 82, 255}
            }
        }) {
            if (!postsLoaded) {
                CLAY_TEXT(CLAY_STRING("Loading posts..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            } else {
                for (int i = 0; i < 50; i++) {
                    if (posts[i].postText != NULL) {
                        post_component(&posts[i]);
                    }
                }
                CLAY({
                    .id = CLAY_ID("load_more_button"),
                    .layout = {
                        .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH), CLAY_SIZING_GROW(0)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .padding = { .bottom = 10, .top = 10 },
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
                    },
                    .border = {.width = {.top = 1}, .color = {46, 64, 82, 255}}
                }) {
                    Clay_OnHover(onLoadMorePosts, 0);
                    CLAY_TEXT(CLAY_STRING("Load more posts"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
                }
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

static void main_update(void) {
    
}

static void main_unload(void) {
    if (iconsSheet) {
        C2D_SpriteSheetFree(iconsSheet);
    }
    if (otherIconsSheet) {
        C2D_SpriteSheetFree(otherIconsSheet);
    }

    runPostLoadingThread = false;
    svcCloseHandle(threadReq);
}

Scene* get_main_scene(void) {
    main_scene.init = main_init;
    main_scene.update = main_update;
    main_scene.layout = main_layout;
    main_scene.unload = main_unload;
    
    return &main_scene;
} 