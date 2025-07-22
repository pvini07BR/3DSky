#include "components/post.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "theming.h"

void post_init(Post* post, void *feedPtr) {
    post->feedPtr = feedPtr;

    post->uri = NULL;
    post->createdAt = NULL;
    post->indexedAt = NULL;
    post->displayName = NULL;
    post->handle = NULL;
    post->avatarUrl = NULL;
    post->postText = NULL;
    post->avatarImage = NULL;
}

void post_component(Post* post, void (*onHoverFunction)(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData), bool disable) {
    CLAY({
        .layout = {
            .padding = CLAY_PADDING_ALL(10),
            .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .childGap = 4
        }
    }) {
        if (!disable) Clay_OnHover(onHoverFunction, (intptr_t)post);

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