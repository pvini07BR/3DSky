#include "components/feed.h"

#include "jansson.h"
#include "string_utils.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData);
void feed_load_posts(Feed* feed, json_t *root);

void feed_load_timeline(Feed *feed) {
    feed->type = FEED_TYPE_TIMELINE;
    feed->loaded = false;

    bs_client_response_t* response = bs_client_timeline_get(&feed->pagOpts);
    if (response->err_code != 0) {
        if (response->err_msg != NULL) {
            fprintf(stderr, "Failed loading timeline: %s\n", response->err_msg);
            bs_client_response_free(response);
        }
    } else {
        json_error_t error;
        json_t* root = json_loads(response->resp, 0, &error);
        bs_client_response_free(response);

        if (!root) {
            fprintf(stderr, "Error parsing timeline string at line %d: %s\n", error.line, error.text);
        } else {            
            feed_load_posts(feed, root);
    
            json_decref(root);
        }
    }

    feed->loaded = true;
}

void feed_load_posts(Feed* feed, json_t* root) {
    json_t* cursor_json = json_object_get(root, "cursor");

    const char* cursor = json_string_value(cursor_json);

    json_t *posts_array = json_object_get(root, "feed");
    if (!json_is_array(posts_array)) { 
        fprintf(stderr, "Error: Feed is not an array.\n");
        json_decref(root);
        return;
    }

    if (feed->pagOpts.cursor) {
        free(feed->pagOpts.cursor);
    }
    feed->pagOpts.cursor = cursor ? strdup(cursor) : NULL;

    Clay_Citro2d_ClearTextCacheAndBuffer();

    for (size_t i = 0; i < json_array_size(posts_array); i++) {
        if (i >= 50) break;

        if (feed->posts[i].displayName) {
            free(feed->posts[i].displayName);
        }
        if (feed->posts[i].handle) {
            free(feed->posts[i].handle);
        }
        if (feed->posts[i].postText) {
            free(feed->posts[i].postText);
        }
        if (feed->posts[i].avatarUrl) {
            free(feed->posts[i].avatarUrl);
        }

        json_t* post = json_array_get(posts_array, i);
        
        json_t* post_data = json_object_get(post, "post");
        json_t* author = json_object_get(post_data, "author");

        const char* displayName = json_string_value(json_object_get(author, "displayName"));
        const char* handle = json_string_value(json_object_get(author, "handle"));

        json_t* record = json_object_get(post_data, "record");

        const char* postText = json_string_value(json_object_get(record, "text"));

        const char* avatarUrl = json_string_value(json_object_get(author, "avatar"));
        char* avatar_thumbnail_url = replace_substring(avatarUrl, "avatar", "avatar_thumbnail");

        feed->posts[i].displayName = displayName ? strdup(displayName) : NULL;
        feed->posts[i].handle = handle ? strdup(handle) : NULL;
        feed->posts[i].postText = postText ? strdup(postText) : NULL;
        feed->posts[i].avatarUrl = avatar_thumbnail_url ? strdup(avatar_thumbnail_url) : NULL;

        feed->posts[i].avatarImage = NULL;
    }
}

void feed_layout(Feed* data) {
    CLAY({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        }
    }) {
        CLAY({
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(TOP_BOTTOM_DIFF-1), CLAY_SIZING_GROW(0) }
            }
        });

        CLAY((Clay_ElementDeclaration){
            .id = CLAY_ID("timelineScroll"),
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH+2), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .padding = { .top = TOP_HEIGHT-1 },
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER},
            },
            .clip = {
                .horizontal = true,
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
                        .top = 0
                    },
                    .color = {46, 64, 82, 255}
                }
            });
            
            if (data->loaded) {
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
            } else {
                CLAY({
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(BOTTOM_HEIGHT-40)},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING("Loading posts..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
                }
            }
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

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) { return; }
    Feed* data = (Feed*)userData;
    if (data == NULL) { return; }
    
    if (data->loaded) {
        if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
            switch (data->type) {
                case FEED_TYPE_TIMELINE: {
                    feed_load_timeline(data);
                } break;
                default: {

                } break;
            }
        }
    }
}