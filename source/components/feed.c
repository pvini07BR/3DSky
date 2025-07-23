#include "components/feed.h"

#include "3ds/services/hid.h"
#include "avatar_img_cache.h"
#include "c2d/base.h"
#include "components/post.h"
#include "components/post_view.h"
#include "jansson.h"
#include "string_utils.h"
#include "theming.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"
#include "thread_pool.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

bool load_more_posts_pressed = false;

void onLoadMorePosts(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) { return; }
    Feed* data = (Feed*)userData;
    if (data == NULL) { return; }
    
    if (data->loaded) {
        if (!load_more_posts_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
            load_more_posts_pressed = true;
        }

        if (load_more_posts_pressed && hidKeysUp() & KEY_TOUCH) {
            feed_load(data, false);
            load_more_posts_pressed = false;
        }
    }
}

void set_json_string_field(json_t* obj, char** field) {
    if (!obj) {
        fprintf(stderr, "Error: JSON object is null.\n");
        return;
    };
    if (*field) free(*field);
    const char* str = json_string_value(obj);
    if (str != NULL) {
        *field = strdup(str);
    } else {
        fprintf(stderr, "Error: JSON string value is null.\n");
        *field = NULL;
    }
}

void feed_parse_posts(Feed* feed, json_t* root) {
    json_t *posts_array = json_object_get(root, "feed");
    if (!json_is_array(posts_array)) { 
        fprintf(stderr, "Error: Feed is not an array.\n");
        json_decref(root);
        return;
    }
    
    set_json_string_field(json_object_get(root, "cursor"), &feed->pagOpts.cursor);

    Clay_Citro2d_ClearTextCacheAndBuffer();

    for (size_t i = 0; i < json_array_size(posts_array); i++) {
        if (i >= 50) break;
        if (feed->stopLoadingThread) break;

        feed->posts[i].avatarImage = NULL;

        json_t* post = json_array_get(posts_array, i);
        json_t* post_data = json_object_get(post, "post");

        set_json_string_field(json_object_get(post_data, "uri"), &feed->posts[i].uri);
        set_json_string_field(json_object_get(post_data, "indexedAt"), &feed->posts[i].indexedAt);

        // Getting likes, reposts, replies and quotes counts
        feed->posts[i].replyCount = json_integer_value(json_object_get(post_data, "replyCount"));
        feed->posts[i].repostCount = json_integer_value(json_object_get(post_data, "repostCount"));
        feed->posts[i].likeCount = json_integer_value(json_object_get(post_data, "likeCount"));
        feed->posts[i].quoteCount = json_integer_value(json_object_get(post_data, "quoteCount"));

        post_update_counts(&feed->posts[i]);

        // "Author" section of a post
        json_t* author = json_object_get(post_data, "author");

        set_json_string_field(json_object_get(author, "displayName"), &feed->posts[i].displayName);
        set_json_string_field(json_object_get(author, "handle"), &feed->posts[i].handle);
        
        char* avatarUrl = NULL;
        set_json_string_field(json_object_get(author, "avatar"), &avatarUrl);
        // Unlike the other fields, the avatar thumbnail URL is
        // not bound to a JSON root, so we can just directly assign the pointer
        // Don't worry, I'm sure it will be freed later
        feed->posts[i].avatarUrl = replace_substring(avatarUrl, "avatar", "avatar_thumbnail");
        
        // "Record" section of a post
        json_t* record = json_object_get(post_data, "record");

        set_json_string_field(json_object_get(record, "text"), &feed->posts[i].postText);
        set_json_string_field(json_object_get(record, "createdAt"), &feed->posts[i].createdAt);

        // "Embed" section of a post
        json_t* embed = json_object_get(post_data, "embed");
        feed->posts[i].hasEmbed = embed != NULL;
    }
}

void avatar_loading_thread(void* args) {
    if (args == NULL) return;
    Feed* feed = (Feed*)args;
    if (feed == NULL) return;

    char* uniqueUrls[50] = {0};
    int uniqueCount = 0;

    for (int i = 0; i < 50; i++) {
        if (feed->stopAvatarThread) break;
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
        if (feed->stopAvatarThread) break;
        C2D_Image* img = avatar_img_cache_get_or_download_image(uniqueUrls[i], 32, 32);
        if (img) {
            for (int j = 0; j < 50; j++) {
                if (feed->posts[j].avatarUrl && strcmp(feed->posts[j].avatarUrl, uniqueUrls[i]) == 0) {
                    feed->posts[j].avatarImage = img;
                }
            }
        }
    }

    printf("Finished loading avatars.\n");
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
            if (feed->did == NULL || strlen(feed->did) == 0) {
                fprintf(stderr, "Cannot load the author feed if the did is empty or null. Aborting\n");
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
            feed_parse_posts(feed, root);
    
            json_decref(root);
        }
    }

    feed->loaded = true;

    printf("Finished loading posts.\n");
}

void feed_init(Feed *feed, FeedType feed_type, PostView* postViewPtr, bool disableProfileLoading, C2D_Image* repliesIcon, C2D_Image* repostIcon, C2D_Image* likeIcon) {
    if (feed == NULL) return;

    for (int i = 0; i < 50; i++) post_init(&feed->posts[i], feed, repliesIcon, repostIcon, likeIcon);

    feed->pagOpts = (bs_client_pagination_opts){
        .cursor = NULL,
        .limit = 50
    };
    feed->type = feed_type;
    feed->loaded = false,
    feed->disableProfileLoading = disableProfileLoading;
    feed->postViewPtr = postViewPtr;
    feed->scrollValue = 0.0f;
    feed->prevScroll = 0.0f;
    feed->scrolling = false;
    feed->setScroll = false;
    feed->loadingThreadHandle = (ThreadTaskHandle){0};
    //feed->avatarThreadHandle = NULL;
    feed->stopLoadingThread = false;
    feed->stopAvatarThread = false;
    feed->did = NULL;
}

// This function will create a new thread to load the posts into the feed,
// and will use the appropriate function depending on what feed type has been set
void feed_load(Feed* feed, bool resetCursor) {
    if (!thread_pool_task_is_done(&feed->loadingThreadHandle)) {
        feed->stopLoadingThread = true;
        thread_pool_task_wait(&feed->loadingThreadHandle);
        feed->stopLoadingThread = false;
    }

    if (resetCursor) {
        if (feed->pagOpts.cursor) {
            free(feed->pagOpts.cursor);
            feed->pagOpts.cursor = NULL;
        }
    }
    thread_pool_add_task(post_loading_thread, feed, &feed->loadingThreadHandle);
}

void feed_layout(Feed* data, float top_padding) {
    Clay_ElementId clayId = data->type == FEED_TYPE_TIMELINE ? CLAY_ID("timelineScroll") : CLAY_ID("authorFeedScroll");
    float actualScrollValue = 0.0f;

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
            .color = get_current_theme()->accentColor
        },
    }) {
        actualScrollValue = data->setScroll ? data->scrollValue : Clay_GetScrollOffset().y;
        float deltaScroll = actualScrollValue - data->prevScroll;
        data->prevScroll = actualScrollValue;

        if (deltaScroll <= -3.0f || deltaScroll >= 3.0f) data->scrolling = true;
        if (data->scrolling && hidKeysUp() & KEY_TOUCH) data->scrolling = false;

        if (data->loaded) {
            for (int i = 0; i < 50; i++) {
                if (data->posts[i].postText != NULL) {
                    post_layout(&data->posts[i], data->scrolling, data->disableProfileLoading);
                }
            }
            CLAY((Clay_ElementDeclaration){
                .id = CLAY_ID("load_more_button"),
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = { .bottom = 10, .top = 10 },
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
                },
                .backgroundColor = Clay_Hovered() ? get_current_theme()->accentColor : get_current_theme()->backgroundColor
            }) {
                if (!data->scrolling) Clay_OnHover(onLoadMorePosts, (uintptr_t)data);
                CLAY_TEXT(CLAY_STRING("Load more posts"), CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->textColor, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            }
        } else {
            CLAY({
                .layout = {
                    .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                },
            }) {
                CLAY_TEXT(CLAY_STRING("Loading posts..."), CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->textColor, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
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

void feed_stop_threads(Feed* feed) {
    if (!thread_pool_task_is_done(&feed->loadingThreadHandle)) {
        feed->stopLoadingThread = true;
        thread_pool_task_wait(&feed->loadingThreadHandle);
        feed->stopLoadingThread = false;
    }
}

void feed_free(Feed *feed) {
    feed_stop_threads(feed);

    for (int i = 0; i < 50; i++) {
        post_free(&feed->posts[i]);
    }
}