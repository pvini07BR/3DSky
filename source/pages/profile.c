#include "pages/profile.h"

#include <stdio.h>

#include "avatar_img_cache.h"
#include "jansson.h"
#include "clay/clay.h"
#include "bluesky/bluesky.h"

#include "defines.h"
#include "string_utils.h"

char* followsText = NULL;

void loadProfileThread(void* args) {
    if (args == NULL) return;
    ProfilePage* data = (ProfilePage*)args;
    if (data == NULL) return;

    bs_client_response_t* resp = bs_client_profile_get(data->handle);
    if (resp->err_code != 0) {
        fprintf(stderr, "An error occurred when loading the profile.\n");
        if (resp->err_msg != NULL)
            fprintf(stderr, "Failed loading profile: %s\n", resp->err_msg);
        
        bs_client_response_free(resp);
        return;
    }
   
    if (resp->resp == NULL) {
        fprintf(stderr, "Error: response is null.\n");
        return;
    }

    json_error_t error;
    json_t* root = json_loads(resp->resp, 0, &error);
    if (!root) {
        fprintf(stderr, "Error parsing profile string at line %d: %s\n", error.line, error.text);
        bs_client_response_free(resp);
        return;
    }
    
    data->displayName = json_string_value(json_object_get(root, "displayName"));
    data->description = json_string_value(json_object_get(root, "description"));

    data->followersCount = json_integer_value(json_object_get(root, "followersCount"));
    data->followsCount = json_integer_value(json_object_get(root, "followsCount"));
    data->postsCount = json_integer_value(json_object_get(root, "postsCount"));

    int len = snprintf(NULL, 0, "%d followers %d follows %d posts", data->followersCount, data->followsCount, data->postsCount);
    followsText = malloc(len + 1);
    if (followsText) {
        snprintf(followsText, len + 1, "%d followers %d follows %d posts", data->followersCount, data->followsCount, data->postsCount); 
    }

    const char* avatarUrl = json_string_value(json_object_get(root, "avatar"));
    if (avatarUrl != NULL) {
        char* avatar_thumbnail_url = replace_substring(avatarUrl, "avatar", "avatar_thumbnail");
        data->avatarImage = avatar_img_cache_get_or_download_image(avatar_thumbnail_url);
    }

    bs_client_response_free(resp);
    data->loaded = true;
}

void profile_page_load(ProfilePage* data, const char* handle) {
    if (data == NULL) return;
    data->handle = handle;
    threadCreate(loadProfileThread, data, (16 * 1024), 0x3f, -2, true);
    data->initialized = true;
}

void profile_page_layout(ProfilePage *data) {
    if (data == NULL) return; 

    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH+2), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        /*
        .scroll = {
            .horizontal = false,
            .vertical = true,
        },
        */
        .clip = {
            .horizontal = false,
            .vertical = true,
            .childOffset = Clay_GetScrollOffset()
        },
        .border = {
            .width = {
                .left = 1,
                .right = 1,
            },
            .color = {46, 64, 82, 255}
        }
    }) {
        if (!data->loaded) {
            CLAY({
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_GROW()
                    },
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER }
                }
            }) {
                CLAY_TEXT(CLAY_STRING("Loading profile..."), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            }
        } else {
            if (data->avatarImage != NULL) {
                CLAY({
                    .layout = {
                    .sizing = {CLAY_SIZING_FIXED(32), CLAY_SIZING_FIXED(32)},
                },
                    .image = {
                        .imageData = data->avatarImage,
                        .sourceDimensions = (Clay_Dimensions) { .width = 32, .height = 32 },
                    }
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

            if (followsText != NULL) {
                Clay_String str = (Clay_String){.chars = followsText, .length = strlen(followsText)};
                CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0, })); 
            }

            if (data->description != NULL) {
                Clay_String str = (Clay_String) { .chars = data->description, .length = strlen(data->description) };
                CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 15, .fontId = 0, }));
            }
        }        
    }
}

void profile_page_free() {
    free(followsText);
}
