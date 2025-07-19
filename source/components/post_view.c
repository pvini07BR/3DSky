#include "components/post_view.h"
#include "3ds/services/hid.h"
#include "components/post.h"

void post_view_init(PostView *data) {
    if (data == NULL) return;
    data->post = NULL;
    data->opened = false;
}

void post_view_set(PostView* data, Post* post) {
    if (data == NULL) return;
    if (post == NULL) return;

    data->post = post;
    data->opened = true;
}

void post_view_layout(PostView* data) {
    if (!data->opened) return;
    CLAY({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .backgroundColor = (Clay_Color){0.0f, 0.0f, 0.0f, 1.0f}
    }) {
        if (data->post) {
            Clay_String postText = (Clay_String) { .chars = data->post->postText, .length = strlen(data->post->postText) };
            CLAY_TEXT(
                postText,
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 255, 255, 255},
                    .fontSize = 15,
                    .fontId = 0
                })
            );
        } else {
            CLAY_TEXT(
                CLAY_STRING("There's no way I can show a NULL post."),
                CLAY_TEXT_CONFIG({
                    .textColor = {255, 255, 255, 255},
                    .fontSize = 15,
                    .fontId = 0
                })
            );
        }
    }
}

void post_view_input(PostView *data) {
    u32 kDown = hidKeysDown();
    if (kDown & KEY_B && data->opened) {
        data->opened = false;
    }
}