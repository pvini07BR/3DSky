#include "components/post.h"
#include <stdlib.h>
#include <string.h>
#include "defines.h"

typedef struct {
    Post* post;
    void (*callback)(void* data, Post* post);
    void* context;
} PostCallbackData;

void onPostClicked(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    /*
    if (userData == 0) { return; }
    PostCallbackData* data = (PostCallbackData*)userData;
    if (data == NULL) { return; }
    
    if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
        if (data->callback && data->context && data->post) {
            data->callback(data->context, data->post);
        }
    }
    */
}

void post_component(Post* post, void (*post_open_callback)(void*, Post*), void* context) {
    CLAY({
        .layout = {
            .padding = CLAY_PADDING_ALL(10),
            .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = 4
        }
    }) {
        PostCallbackData callbackData = {
            .post = post,
            .callback = post_open_callback,
            .context = context
        };

        Clay_OnHover(onPostClicked, (uintptr_t)&callbackData);

        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(32), CLAY_SIZING_FIXED(32)},
            },
            .image = {
                .imageData = post->avatarImage,
            },
            .aspectRatio = { 32.0f / 32.0f }
        });
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
                Clay_String displayName = (Clay_String) { .chars = post->displayName, .length = strlen(post->displayName) };
                Clay_String handle = (Clay_String) { .chars = post->handle, .length = strlen(post->handle) };

                CLAY_TEXT(displayName, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0, .wrapMode = CLAY_TEXT_WRAP_NONE }));
                CLAY_TEXT(handle, CLAY_TEXT_CONFIG({ .textColor = {128, 128, 128, 255}, .fontSize = 15, .fontId = 0, .wrapMode = CLAY_TEXT_WRAP_NONE }));
            }
            Clay_String postText = (Clay_String) { .chars = post->postText, .length = strlen(post->postText) };
            CLAY_TEXT(
                postText,
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 255, 255, 255},
                    .fontSize = 15,
                    .fontId = 0
                })
            );
        }
    }
}

void post_free(Post *post) {
    if (post == NULL) return;

    if (post->displayName) {
        free(post->displayName);
    }
    if (post->handle) {
        free(post->handle);
    }
    if (post->postText) {
        free(post->postText);
    }
    if (post->avatarUrl) {
        free(post->avatarUrl);
    }
}