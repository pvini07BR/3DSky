#include "components/post_view.h"
#include "3ds/services/hid.h"
#include "components/post.h"
#include "defines.h"
#include "stdio.h"
#include "theming.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"

void post_view_init(PostView *data) {
    if (data == NULL) return;
    data->post = NULL;
    data->opened = false;
}

void post_view_set(PostView* data, Post* post) {
    if (data == NULL) return;
    if (post == NULL) return;

    data->postViewScroll = 0.0f;
    data->post = post;
    data->opened = true;
}

void post_view_layout(PostView* data) {
    if (data == NULL) return;
    if (!data->opened) return;

    CLAY({
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(TOP_WIDTH), CLAY_SIZING_FIXED(TOP_HEIGHT) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        CLAY({
            .id = CLAY_ID("postViewHeader"),
            .layout = {
                .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIT() },
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                .padding = CLAY_PADDING_ALL(8),
                .childGap = 16
            },
            .border = {
                .width = {
                    .bottom = 1
                },
                .color = get_current_theme()->accentColor
            }
        }) {
            CLAY_TEXT(CLAY_STRING("Post"), CLAY_TEXT_CONFIG({
                .textColor = get_current_theme()->textColor,
                .fontSize = 24,
                .fontId = 0
            }));

            CLAY_TEXT(CLAY_STRING("(Press B to go back.)"), CLAY_TEXT_CONFIG({
                .textColor = get_current_theme()->diminishedTextColor,
                .fontSize = 16,
                .fontId = 0
            }));
        }

        if (data->post == NULL) {
            CLAY_TEXT(
                CLAY_STRING("There is no way I'm going to show a NULL post."),
                CLAY_TEXT_CONFIG({
                    .textColor = get_current_theme()->textColor,
                    .fontSize = 20,
                    .fontId = 0
                })
            );
        } else {
            CLAY({
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(8),
                    .childGap = 8
                },
                .clip = {
                    .horizontal = false,
                    .vertical = true,
                    .childOffset = {
                        .x = 0.0f,
                        .y = data->postViewScroll
                    }
                }
            }) {
                CLAY({
                    .id = CLAY_ID("containerForMeasuring"),
                    .layout = {
                        .sizing = { CLAY_SIZING_FIT(), CLAY_SIZING_FIT() },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }) {
                    CLAY({
                        .layout = {
                            .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            .childGap = 8
                        }
                    }) {
                        CLAY({
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(32), CLAY_SIZING_FIXED(32)},
                            },
                            .image = {
                                .imageData = data->post->avatarImage,
                            },
                            .aspectRatio = { 32.0f / 32.0f }
                        });
        
                        CLAY({
                            .layout = {
                                .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM
                            }
                        }) {
                            if (data->post->displayName) {
                                Clay_String displayName = (Clay_String) { .chars = data->post->displayName, .length = strlen(data->post->displayName) };
                                CLAY_TEXT(displayName, CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->textColor, .fontSize = 15, .fontId = 0, .wrapMode = CLAY_TEXT_WRAP_NONE }));
                            }
                            if (data->post->handle) {
                                Clay_String handle = (Clay_String) { .chars = data->post->handle, .length = strlen(data->post->handle) };
                                CLAY_TEXT(handle, CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->diminishedTextColor, .fontSize = 15, .fontId = 0, .wrapMode = CLAY_TEXT_WRAP_NONE }));
                            }
                        }
                    }
        
                    if (data->post->postText) {
                        Clay_String postText = (Clay_String) { .chars = data->post->postText, .length = strlen(data->post->postText) };
                        CLAY_TEXT(
                            postText,
                            CLAY_TEXT_CONFIG({
                                .textColor = get_current_theme()->textColor,
                                .fontSize = 20,
                                .fontId = 0
                            })
                        );
                    }
                }
            }
        }
    }
}

void post_view_input(PostView *data, float deltaTime) {
    if (data == NULL) return;
    if (!data->opened) return;

    u32 kDown = hidKeysDown();
    if (kDown & KEY_B && data->opened) {
        data->opened = false;
    }

    
    Clay_ElementData headerData = Clay_GetElementData(CLAY_ID("postViewHeader"));
    Clay_ElementData containerData = Clay_GetElementData(CLAY_ID("containerForMeasuring"));

    float scrollAreaHeight = 0.0f;
    if (headerData.found) scrollAreaHeight = (TOP_HEIGHT - headerData.boundingBox.height - 16);
    if (containerData.found) {
        if (containerData.boundingBox.height < scrollAreaHeight) {
            return;
        }
    }

    circlePosition circlePos;
    hidCircleRead(&circlePos);
    if (circlePos.dy > 10 || circlePos.dy < -10) {
        data->postViewScroll += circlePos.dy * deltaTime * 2.0f;
        if (data->postViewScroll > 0.0f) data->postViewScroll = 0.0f;

        if (containerData.found && headerData.found) {
            float bound = -containerData.boundingBox.height + scrollAreaHeight;    // 16 is to account for the padding
            if (data->postViewScroll < bound) data->postViewScroll = bound;
        }
    }
}