#include <3ds.h>
#include "bluesky/bluesky.h"
#include "jansson.h"

#include "pages/timeline.h"

#include "defines.h"

void loadPostsThread(void* args) {
    if (args == NULL) {
        return;
    }

    TimelinePage* data = (TimelinePage*)args;
    if (data == NULL) {
        return;
    }

    data->postsLoaded = false;

    bs_client_pagination_opts opts = {
        .cursor = (char*)data->cursor,
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
            data->cursor = json_string_value(cursor_json);
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

            data->posts[i].displayName = json_string_value(json_object_get(author, "displayName"));
            data->posts[i].handle = json_string_value(json_object_get(author, "handle"));

            json_t* record = json_object_get(post_data, "record");

            data->posts[i].postText = json_string_value(json_object_get(record, "text"));
        }
    }
    bs_client_response_free(response);
    data->postsLoaded = true;
}

void timeline_page_load_posts(TimelinePage* data) {
    if (data == NULL){return;}
    threadCreate(loadPostsThread, data, (16 * 1024), 0x3f, -2, true);
}

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) { return; }
    TimelinePage* data = (TimelinePage*)userData;
    if (data == NULL) { return; }

    if (data->postsLoaded) {
        if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            timeline_page_load_posts(data);
        }
    }
}

void timeline_page_layout(TimelinePage *data) {
    if (data == NULL) { return; }

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
        if (!data->postsLoaded) {
            CLAY_TEXT(CLAY_STRING("Loading posts..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
        } else {
            for (int i = 0; i < 50; i++) {
                if (data->posts[i].postText != NULL) {
                    post_component(&data->posts[i]);
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
                Clay_OnHover(onLoadMorePosts, (uintptr_t)data);
                CLAY_TEXT(CLAY_STRING("Load more posts"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            }
        }
    }
}