#include <3ds.h>
#include "3ds/thread.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"
#include "thirdparty/bluesky/bluesky.h"
#include "jansson.h"
#include "pages/timeline.h"

#include "string_utils.h"
#include "defines.h"
#include "avatar_img_cache.h"

bool stopAvatarThread = false;
bool stopPostLoadingThread = false;

Thread postLoadingThreadHnd = NULL;
Thread avatarThreadHnd = NULL;

// TODO: Try to somehow make all posts using the same avatar URL load instantly after downloading the image,
// and then go to download the next one.
void downloadAvatarsThread(void* args) {
    if (args == NULL) return;
    TimelinePage* data = (TimelinePage*)args;
    if (data == NULL) return;

    for (int i = 0; i < 50; i++) {
        if (stopAvatarThread) break;

        if (data->posts[i].avatarImage != NULL) continue;
        data->posts[i].avatarImage = avatar_img_cache_get_or_download_image(data->posts[i].avatarUrl, 32, 32);
    }
}

// TODO: Try to optimize JSON parsing as the JSON strings returned by the Bluesky API are huge.
void loadPostsThread(void* args) {
    if (args == NULL) return;

    TimelinePage* data = (TimelinePage*)args;
    if (data == NULL) return;

    data->postsLoaded = false;

    bs_client_pagination_opts opts = {
        .cursor = (char*)data->cursor,
        .limit = 50
    };

    bs_client_response_t* response = bs_client_timeline_get(&opts);
    if (response->err_code != 0) {
        if (response->err_msg != NULL)
            fprintf(stderr, "Failed loading timeline: %s\n", response->err_msg);
        
        bs_client_response_free(response);
        return;
    }
                        
    json_error_t error;
    json_t* root = json_loads(response->resp, 0, &error);
    if (!root) {
        fprintf(stderr, "Error parsing timeline string at line %d: %s\n", error.line, error.text);
        bs_client_response_free(response);
        return;
    }

    json_t* cursor_json = json_object_get(root, "cursor");
    data->cursor = json_string_value(cursor_json);

    json_t *posts_array = json_object_get(root, "feed");
    if (!json_is_array(posts_array)) { 
        fprintf(stderr, "Error: Feed is not an array.\n");
        bs_client_response_free(response);
        return;
    }

    for (size_t i = 0; i < json_array_size(posts_array); i++) {
        if (i >= 50) { break; }
        if (stopPostLoadingThread) break;

        json_t* post = json_array_get(posts_array, i);
        
        json_t* post_data = json_object_get(post, "post");
        json_t* author = json_object_get(post_data, "author");

        data->posts[i].displayName = json_string_value(json_object_get(author, "displayName"));
        data->posts[i].handle = json_string_value(json_object_get(author, "handle"));

        json_t* record = json_object_get(post_data, "record");

        data->posts[i].postText = json_string_value(json_object_get(record, "text"));

        const char* avatarUrl = json_string_value(json_object_get(author, "avatar"));
        char* avatar_thumbnail_url = replace_substring(avatarUrl, "avatar", "avatar_thumbnail");

        data->posts[i].avatarUrl = avatar_thumbnail_url;
        data->posts[i].avatarImage = NULL;
    }

    bs_client_response_free(response);
    data->postsLoaded = true;

    // Create new thread for downloading the avatar images
    // after loading the posts.
    if (data->postsLoaded && !stopPostLoadingThread) {
        stopAvatarThread = false;
        avatarThreadHnd = threadCreate(downloadAvatarsThread, data, (16 * 1024), 0x3f, -2, true);
    }
}

void timeline_page_load_posts(TimelinePage* data) {
    if (data == NULL){return;}
    Clay_Citro2d_ClearTextCacheAndBuffer();
    postLoadingThreadHnd = threadCreate(loadPostsThread, data, (16 * 1024), 0x3f, -2, true);
    data->initialized = true;
}

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) { return; }
    TimelinePage* data = (TimelinePage*)userData;
    if (data == NULL) { return; }
    
    if (data->postsLoaded) {
        if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            timeline_stop_avatar_thread();
            timeline_page_load_posts(data);
        }
    }
}

void timeline_page_layout(TimelinePage *data) {
    if (data == NULL) { return; }

    if (data->postsLoaded) {
        CLAY((Clay_ElementDeclaration){
            .id = CLAY_ID("timelineScroll"),
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH+2), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = { .top = TOP_HEIGHT-2 },
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
            },
            .clip = {
                .horizontal = false,
                .vertical = true,
                .childOffset = {
                    .x = 0.0f,
                    .y = data->setScroll ? data->scrollValue : Clay_GetScrollOffset().y
                }
            },
            .border = {
                .width = {
                    .left = 1,
                    .right = 1,
                    .betweenChildren = CLAY_TOP_TO_BOTTOM
                },
                .color = {46, 64, 82, 255}
            },
        }) {
            CLAY((Clay_ElementDeclaration){
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIXED(BOTTOM_WIDTH),
                        .height = CLAY_SIZING_FIXED(1)
                    }
                },
                .border = {
                    .width = {
                        .top = 0.5
                    },
                    .color = {46, 64, 82, 255}
                }
            });

            for (int i = 0; i < 50; i++) {
                if (data->posts[i].postText != NULL) {
                    post_component(&data->posts[i]);
                }
            }
            CLAY((Clay_ElementDeclaration){
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
    } else {
        CLAY((Clay_ElementDeclaration){
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIXED(BOTTOM_WIDTH+2), .height = CLAY_SIZING_GROW(0)},
                .padding = { .top = TOP_HEIGHT },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
            },
            .border = {
                .width = {
                    .left = 1,
                    .right = 1,
                    .betweenChildren = CLAY_TOP_TO_BOTTOM
                },
                .color = {46, 64, 82, 255}
            },
        }) {
            CLAY_TEXT(CLAY_STRING("Loading posts..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
        }
    }

    Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(CLAY_ID("timelineScroll"));
    if (scrollData.found) {
        if (scrollData.scrollPosition != NULL) {
            if (data->setScroll) {
                scrollData.scrollPosition->y = data->scrollValue;
                data->setScroll = false;
            }

            data->scrollValue = scrollData.scrollPosition->y;
        }
    }
}

void timeline_free() {
    timeline_stop_avatar_thread();

    if (postLoadingThreadHnd) {
        stopPostLoadingThread = true;
        threadJoin(postLoadingThreadHnd, U64_MAX);
    }
}

void timeline_stop_avatar_thread() {
    if (avatarThreadHnd) {
        stopAvatarThread = true;
        threadJoin(avatarThreadHnd, U64_MAX);
    }
}