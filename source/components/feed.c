#include "components/feed.h"

#include "avatar_img_cache.h"
#include "c2d/base.h"
#include "components/post.h"
#include "components/post_view.h"
#include "jansson.h"
#include "string_utils.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"
#include <stdint.h>
#include <string.h>

bool load_more_posts_pressed = false;
bool post_pressed = false;

void onPostHover(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) return;
    Post* data = (Post*)userData;
    if (data == NULL) return;
    Feed* feedPtr = (Feed*)data->feedPtr;
    if (feedPtr == NULL) return;
    if (feedPtr->postViewPtr == NULL) return;

    if (!post_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
        post_pressed = true;
    }

    if (post_pressed && hidKeysUp() & KEY_TOUCH) {

        feedPtr->setScroll = true;
        post_view_set(feedPtr->postViewPtr, data);
        post_pressed = false;
    }
}

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) { return; }
    Feed* data = (Feed*)userData;
    if (data == NULL) { return; }
    
    if (data->loaded) {
        if (!load_more_posts_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
            load_more_posts_pressed = true;
        }

        if (load_more_posts_pressed && hidKeysUp() & KEY_TOUCH) {
            feed_load(data);
            load_more_posts_pressed = false;
        }
    }
}

void feed_load_posts(Feed* feed, json_t* root) {
    json_t *posts_array = json_object_get(root, "feed");
    if (!json_is_array(posts_array)) { 
        fprintf(stderr, "Error: Feed is not an array.\n");
        json_decref(root);
        return;
    }
    
    json_t* cursor_json = json_object_get(root, "cursor");
    const char* cursor = json_string_value(cursor_json);
    if (cursor) {
        if (feed->pagOpts.cursor) free(feed->pagOpts.cursor); 
        feed->pagOpts.cursor = strdup(cursor);
    }

    Clay_Citro2d_ClearTextCacheAndBuffer();

    for (size_t i = 0; i < json_array_size(posts_array); i++) {
        if (i >= 50) break;
        if (feed->stopLoadingThread) break;

        if (feed->posts[i].displayName) free(feed->posts[i].displayName);
        if (feed->posts[i].handle) free(feed->posts[i].handle);
        if (feed->posts[i].postText) free(feed->posts[i].postText);
        if (feed->posts[i].avatarUrl) free(feed->posts[i].avatarUrl);

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

void avatar_loading_thread(void* args) {
    if (args == NULL) return;
    Feed* feed = (Feed*)args;
    if (feed == NULL) return;

    char* uniqueUrls[50] = {0};
    int uniqueCount = 0;

    for (int i = 0; i < 50; i++) {
        if (feed->stopAvatarThread) return;
        if (feed->posts[i].avatarUrl && feed->posts[i].avatarImage == NULL) {
            int found = 0;
            for (int j = 0; j < uniqueCount; j++) {
                if (strcmp(feed->posts[i].avatarUrl, uniqueUrls[j]) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                uniqueUrls[uniqueCount++] = feed->posts[i].avatarUrl;
            }
        }
    }

    for (int i = 0; i < uniqueCount; i++) {
        if (feed->stopAvatarThread) return;
        C2D_Image* img = avatar_img_cache_get_or_download_image(uniqueUrls[i], 32, 32);
        if (img) {
            for (int j = 0; j < 50; j++) {
                if (feed->posts[j].avatarUrl && strcmp(feed->posts[j].avatarUrl, uniqueUrls[i]) == 0) {
                    feed->posts[j].avatarImage = img;
                }
            }
        }
    }
}

void post_loading_thread(void* args) {
    if (args == NULL) return;
    Feed* feed = (Feed*)args;
    if (feed == NULL) return;

    feed->loaded = false;

    bs_client_response_t* response = NULL;
    switch (feed->type) {
        case FEED_TYPE_TIMELINE: {
            response = bs_client_timeline_get(&feed->pagOpts);
        } break;
        case FEED_TYPE_AUTHOR: {
            if (feed->did == NULL) {
                fprintf(stderr, "Cannot load the author feed if the did is null. Aborting\n");
                feed->loaded = true;
                return;
            }
            response = bs_client_author_feed_get(feed->did, &feed->pagOpts);
        } break;
        default: {
            fprintf(stderr, "Unhandled feed type. Aborting\n");
            feed->loaded = true;
            return;
        };
    }

    if (response == NULL) {
        fprintf(stderr, "Response is null. Cannot procceed with that. Aborting\n");
        feed->loaded = true;
        return;
    }
    
    if (response->err_code != 0) {
        if (response->err_msg != NULL) {
            if (feed->type == FEED_TYPE_TIMELINE)
                fprintf(stderr, "Failed loading timeline: %s\n", response->err_msg);
            else
                fprintf(stderr, "Failed loading author posts: %s\n", response->err_msg);
            bs_client_response_free(response);
        }
    } else {
        json_error_t error;
        json_t* root = json_loads(response->resp, 0, &error);
        bs_client_response_free(response);

        if (!root) {
            if (feed->type == FEED_TYPE_TIMELINE)
                fprintf(stderr, "Error parsing timeline string at line %d: %s\n", error.line, error.text);
            else
                fprintf(stderr, "Error parsing author posts string at line %d: %s\n", error.line, error.text);
        } else {            
            feed_load_posts(feed, root);
    
            json_decref(root);

            feed->avatarThreadHandle = threadCreate(avatar_loading_thread, feed, (16 * 1024), 0x3f, -2, true);
        }
    }

    feed->loaded = true;
}

void feed_init(Feed *feed, FeedType feed_type, PostView* postViewPtr) {
    if (feed == NULL) return;

    for (int i = 0; i < 50; i++) {
        feed->posts[i].feedPtr = feed;
    }
    feed->pagOpts = (bs_client_pagination_opts){
        .cursor = NULL,
        .limit = 50
    };
    feed->type = feed_type;
    feed->loaded = false,
    feed->postViewPtr = postViewPtr;
    feed->scrollValue = 0.0f;
    feed->setScroll = false;
    feed->loadingThreadHandle = NULL;
    feed->avatarThreadHandle = NULL;
    feed->stopLoadingThread = false;
    feed->stopAvatarThread = false;
    feed->did = NULL;
}

// This function will create a new thread to load the posts into the feed,
// and will use the appropriate function depending on what feed type has been set
void feed_load(Feed* feed) {
    if (feed->avatarThreadHandle) {
        feed->stopAvatarThread = true;
        threadJoin(feed->avatarThreadHandle, U64_MAX);
    }
    if (feed->loadingThreadHandle) {
        feed->stopLoadingThread = true;
        threadJoin(feed->loadingThreadHandle, U64_MAX);
    }
    feed->loadingThreadHandle = threadCreate(post_loading_thread, feed, (16 * 1024), 0x3f, -2, true);
}

void feed_layout(Feed* data, float top_padding) {
    Clay_ElementId clayId = data->type == FEED_TYPE_TIMELINE ? CLAY_ID("timelineScroll") : CLAY_ID("authorFeedScroll");

    CLAY((Clay_ElementDeclaration){
        .id = clayId,
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(BOTTOM_WIDTH), CLAY_SIZING_GROW(0) },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .padding = { .top = top_padding },
            .childAlignment = {.x = CLAY_ALIGN_X_LEFT},
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
                .betweenChildren = CLAY_TOP_TO_BOTTOM
            },
            .color = {46, 64, 82, 255}
        },
    }) {
        
        if (data->loaded) {
            for (int i = 0; i < 50; i++) {
                if (data->posts[i].postText != NULL) {
                    post_component(&data->posts[i], onPostHover);
                }
            }
            CLAY((Clay_ElementDeclaration){
                .id = CLAY_ID("load_more_button"),
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = { .bottom = 10, .top = 10 },
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
                },
                .backgroundColor = Clay_Hovered() ? (Clay_Color){46, 64, 82, 255} : (Clay_Color){22, 30, 39, 255}
            }) {
                Clay_OnHover(onLoadMorePosts, (uintptr_t)data);
                CLAY_TEXT(CLAY_STRING("Load more posts"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            }
        } else {
            CLAY({
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                },
            }) {
                CLAY_TEXT(CLAY_STRING("Loading posts..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            }
        }
    }

    Clay_ScrollContainerData scrollData = Clay_GetScrollContainerData(clayId);
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

void feed_free(Feed *feed) {
    if (feed->avatarThreadHandle) {
        feed->stopAvatarThread = true;
        threadJoin(feed->avatarThreadHandle, U64_MAX);
    }
    if (feed->loadingThreadHandle) {
        feed->stopLoadingThread = true;
        threadJoin(feed->loadingThreadHandle, U64_MAX);
    }

    for (int i = 0; i < 50; i++) {
        post_free(&feed->posts[i]);
    }
}