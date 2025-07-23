#include "components/post.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "components/feed.h"
#include "defines.h"
#include "scenes/main_scene.h"
#include "string_utils.h"
#include "theming.h"

bool post_pressed = false;
bool pfp_pressed = false;

void onPfpHover(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (userData == 0) return;
    char* handle = (char*)userData;
    if (handle == NULL) return;

    post_pressed = false;

    if (!pfp_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
        pfp_pressed = true;
    }

    if (pfp_pressed && hidKeysUp() & KEY_TOUCH) {
        main_scene_change_to_profile(handle);
        pfp_pressed = false;
    }
}

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

void post_init(Post* post, void *feedPtr, C2D_Image* repliesIcon, C2D_Image* repostIcon, C2D_Image* likeIcon) {
    post->feedPtr = feedPtr;
    post->repliesIcon = repliesIcon;
    post->repostIcon = repostIcon;
    post->likeIcon = likeIcon;
    post->replyCount = 0;
    post->repostCount = 0;
    post->likeCount = 0;
    post->quoteCount = 0;
    post->hasEmbed = false;
    post->uri = NULL;
    post->createdAt = NULL;
    post->indexedAt = NULL;
    post->displayName = NULL;
    post->handle = NULL;
    post->avatarUrl = NULL;
    post->postText = NULL;
    post->avatarImage = NULL;
    post_update_counts(post);
}

void postIconLayout(C2D_Image* icon, const char* str) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = 4
        },
    }) {
        CLAY({
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_FIXED(16),
                    .height = CLAY_SIZING_FIXED(16) 
                }
            },
            .backgroundColor = get_current_theme()->diminishedTextColor,
            .image = {
                .imageData = icon,
            },
            .aspectRatio = { 16.0f / 16.0f },
        });
    
        if (str) {
            Clay_String txt = (Clay_String) { .chars = str, .length = strlen(str) };
            CLAY_TEXT(
            txt,
            CLAY_TEXT_CONFIG({
                    .textColor = get_current_theme()->diminishedTextColor,
                    .fontSize = 15,
                    .fontId = 0
                })
            );
        }
    }
}

void post_layout(Post* post, bool disable, bool disableProfileLoading) {
    CLAY({
        .layout = {
            .padding = CLAY_PADDING_ALL(10),
            .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = 4
        }
    }) {
        if (!disable) Clay_OnHover(onPostHover, (intptr_t)post);

        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(32), CLAY_SIZING_FIXED(32)},
            },
            .image = {
                .imageData = post->avatarImage,
            },
            .aspectRatio = { 32.0f / 32.0f }
        }) {
            if (!disable && !disableProfileLoading) Clay_OnHover(onPfpHover, (intptr_t)post->handle);
        }

        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
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
                if (post->displayName) {
                    Clay_String displayName = (Clay_String) { .chars = post->displayName, .length = strlen(post->displayName) };
                    CLAY_TEXT(displayName, CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->textColor, .fontSize = 15, .fontId = 0, .wrapMode = CLAY_TEXT_WRAP_NONE }));
                }
                if (post->handle) {
                    Clay_String handle = (Clay_String) { .chars = post->handle, .length = strlen(post->handle) };
                    CLAY_TEXT(handle, CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->diminishedTextColor, .fontSize = 15, .fontId = 0, .wrapMode = CLAY_TEXT_WRAP_NONE }));
                }
            }

            if (post->postText) {
                Clay_String postText = (Clay_String) { .chars = post->postText, .length = strlen(post->postText) };
                CLAY_TEXT(
                    postText,
                    CLAY_TEXT_CONFIG({
                        .textColor = get_current_theme()->textColor,
                        .fontSize = 15,
                        .fontId = 0
                    })
                );
            }

            if (post->hasEmbed) {
                CLAY_TEXT(
                    CLAY_STRING("This post contains embedded content."),
                    CLAY_TEXT_CONFIG({
                        .textColor = get_current_theme()->diminishedTextColor,
                        .fontSize = 15,
                        .fontId = 0
                    })
                );
            }

            // Post icon buttons or something
            CLAY({
                .layout = {
                    .sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIT()},
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
                    .childGap = 64,
                    .padding = {.top = 10}
                },
            }) {
                postIconLayout(post->repliesIcon, post->replyCountStr);
                postIconLayout(post->repostIcon, post->repostCountStr);
                postIconLayout(post->likeIcon, post->likeCountrStr);
            }
        }
    }
}

void post_update_counts(Post* post) {
    if (post == NULL) return;

    if (post->replyCountStr) free(post->replyCountStr);
    if (post->repostCountStr) free(post->repostCountStr);
    if (post->likeCountrStr) free(post->likeCountrStr);

    post->replyCountStr = formatted_string("%u", post->replyCount);
    post->repostCountStr = formatted_string("%u", post->repostCount);
    post->likeCountrStr = formatted_string("%u", post->likeCount);
}

void post_free(Post *post) {
    if (post == NULL) return;

    if (post->displayName) free(post->displayName);
    if (post->handle) free(post->handle);
    if (post->postText) free(post->postText);
    if (post->avatarUrl) free(post->avatarUrl);
    if (post->replyCountStr) free(post->replyCountStr);
    if (post->repostCountStr) free(post->repostCountStr);
    if (post->likeCountrStr) free(post->likeCountrStr);
}