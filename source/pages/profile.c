#include "pages/profile.h"

#include <stdio.h>

#include "avatar_img_cache.h"
#include "components/feed.h"
#include "jansson.h"
#include "thirdparty/clay/clay.h"
#include "thirdparty/bluesky/bluesky.h"

#include "defines.h"
#include "string_utils.h"

void loadProfileThread(void* args) {
    if (args == NULL) return;
    ProfilePage* data = (ProfilePage*)args;
    if (data == NULL) return;

    bs_client_response_t* resp = bs_client_profile_get(data->handle);
    if (resp->err_code != 0) {
        if (resp->err_msg != NULL)
            fprintf(stderr, "Failed loading profile: %s\n", resp->err_msg);
        
        bs_client_response_free(resp);
        return;
    }

    json_error_t error;
    json_t* root = json_loads(resp->resp, 0, &error);
    if (!root) {
        fprintf(stderr, "Error parsing profile string at line %d: %s\n", error.line, error.text);
        bs_client_response_free(resp);
        return;
    }
    
    bs_client_response_free(resp);

    const char* did = json_string_value(json_object_get(root, "did"));
    if (did) {
        if (data->feed.did) free(data->feed.did);
        data->feed.did = strdup(did);
        feed_load(&data->feed);
    }

    if (data->displayName) free(data->displayName);
    if (data->description) free(data->description);
    
    const char* displayName = json_string_value(json_object_get(root, "displayName"));
    if (displayName) data->displayName = strdup(displayName);
    
    const char* description = json_string_value(json_object_get(root, "description"));
    if (description) data->description = strdup(description);
    
    data->followersCount = json_integer_value(json_object_get(root, "followersCount"));
    data->followsCount = json_integer_value(json_object_get(root, "followsCount"));
    data->postsCount = json_integer_value(json_object_get(root, "postsCount"));
    
    if (data->followsText) free(data->followsText);
    int len = snprintf(NULL, 0, "%d followers %d follows %d posts", data->followersCount, data->followsCount, data->postsCount);
    data->followsText = malloc(len + 1);
    if (data->followsText) {
        snprintf(data->followsText, len + 1, "%d followers %d follows %d posts", data->followersCount, data->followsCount, data->postsCount); 
    }

    const char* avatarUrl = json_string_value(json_object_get(root, "avatar"));
    if (avatarUrl) {
        char* avatar_thumbnail_url = replace_substring(avatarUrl, "avatar", "avatar_thumbnail");
        data->avatarImage = avatar_img_cache_get_or_download_image(avatar_thumbnail_url, 64, 64);
    }

    json_decref(root);

    data->loaded = true;
}

void profile_page_load(ProfilePage* data, const char* handle) {
    if (data == NULL) return;
    data->handle = handle;
    data->loadingThreadHandle = threadCreate(loadProfileThread, data, (16 * 1024), 0x3f, -2, true);
    data->initialized = true;
}

void profile_page_layout(ProfilePage *data) {
    if (data == NULL) return; 

    // Profile information layout on the top and posts on the bottom
    CLAY({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        // Profile info
        CLAY({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(TOP_HEIGHT) },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }
        }) {
            if (!data->loaded) {
                CLAY((Clay_ElementDeclaration){
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_GROW(),
                            .height = CLAY_SIZING_FIXED(TOP_HEIGHT)
                        },
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = !data->loaded ? CLAY_ALIGN_Y_CENTER : CLAY_ALIGN_Y_TOP }
                    }
                }) {
                    CLAY_TEXT(CLAY_STRING("Loading profile..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
                }
            } else {
                if (data->avatarImage != NULL) {
                    CLAY({
                        .layout = {
                        .sizing = {CLAY_SIZING_FIXED(data->avatarImage->subtex->width), CLAY_SIZING_FIXED(data->avatarImage->subtex->height)},
                    },
                        .image = {
                            .imageData = data->avatarImage,
                        },
                        .aspectRatio = { (float)data->avatarImage->subtex->width / (float)data->avatarImage->subtex->height }
                    });
                }
    
                if (data->displayName != NULL) {
                    Clay_String str = (Clay_String) { .chars = data->displayName, .length = strlen(data->displayName) };
                    CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, }));
                }
    
                if (data->handle != NULL) {
                    Clay_String str = (Clay_String) { .chars = data->handle, .length = strlen(data->handle) };
                    CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {128, 128, 128, 255}, .fontSize = 16, .fontId = 0, }));
                }
    
                if (data->followsText != NULL) {
                    Clay_String str = (Clay_String){.chars = data->followsText, .length = strlen(data->followsText)};
                    CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0, })); 
                }
    
                if (data->description != NULL) {
                    Clay_String str = (Clay_String) { .chars = data->description, .length = strlen(data->description) };
                    CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0, }));
                }
            }
        }

        // Temp layout for translating the posts to the bottom
        CLAY({
            .layout = {
                .sizing = { CLAY_SIZING_GROW(), CLAY_SIZING_GROW() },
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            }
        }) {
            // Empty space to push the posts to fit in the bottom screen
            CLAY({
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(TOP_BOTTOM_DIFF), CLAY_SIZING_GROW() }
                }
            });

            // Where the posts will be
            feed_layout(&data->feed, 0.0f, NULL, NULL);
        }
    }
}

void profile_page_free(ProfilePage* data) {
    feed_free(&data->feed);

    if (data->loadingThreadHandle) {
        threadJoin(data->loadingThreadHandle, U64_MAX);
    }
    
    if (data->followsText) free(data->followsText);
    if (data->displayName) free(data->displayName);
    if (data->description) free(data->description);
}
