#include <3ds.h>
#include "bluesky/bluesky.h"
#include "jansson.h"
#include "uthash/uthash.h"

#include "pages/timeline.h"

#include "image_downloader.h"
#include "defines.h"

// Implementing strdup because C99 doesn't have it lol
char* strdup(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char* copy = (char*)malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

char* replace_substring(const char* original, const char* target, const char* replacement) {
    if (!original || !target || !replacement) {
        return NULL;
    }

    const char* pos = strstr(original, target);
    if (!pos) {
        char* result = strdup(original);
        return result;
    }

    size_t prefixLength = pos - original;
    size_t targetLength = strlen(target);
    size_t replacementLength = strlen(replacement);
    size_t originalLength = strlen(original);

    size_t newLength = originalLength - targetLength + replacementLength;
    char* result = (char*)malloc(newLength + 1);
    if (!result) {
        perror("Error allocating memory");
        return NULL;
    }

    strncpy(result, original, prefixLength);
    strcpy(result + prefixLength, replacement);
    strcpy(result + prefixLength + replacementLength, pos + targetLength);

    return result;
}

char* extract_filename(const char* url) {
    if (url == NULL) {
        return NULL;
    }

    const char* lastSlash = strrchr(url, '/');
    if (!lastSlash) {
        return NULL;
    }

    lastSlash++;

    const char* suffix = strstr(lastSlash, "@jpeg");
    if (!suffix) {
        return NULL;
    }

    size_t filenameLength = suffix - lastSlash;

    char* filename = (char*)malloc(filenameLength + 1);
    if (!filename) {
        perror("Error allocating memory");
        return NULL;
    }

    strncpy(filename, lastSlash, filenameLength);
    filename[filenameLength] = '\0';

    return filename;
}

typedef struct {
    char* urlKey;
    C2D_Image image;
    UT_hash_handle hh;
} ImageCache;

ImageCache* imageCache = NULL;

C2D_Image get_or_download_image(const char* url) {
    if (url == NULL) {
        return (C2D_Image){0};
    }

    ImageCache* entry = NULL;
    HASH_FIND_STR(imageCache, url, entry);
    if (entry) {
        printf("Found image\n");
        return entry->image;
    }

    printf("Image not found, downloading...\n");

    entry = (ImageCache*)malloc(sizeof(ImageCache));
    if (!entry) {
        perror("Error allocating memory for image cache entry");
        return (C2D_Image){0};
    }
    entry->urlKey = malloc(strlen(url) + 1);
    if (!entry->urlKey) {
        perror("Error allocating memory for URL key");
        free(entry);
        return (C2D_Image){0};
    }
    strcpy(entry->urlKey, url);
    entry->urlKey[strlen(url)] = '\0';
    entry->image = download_image_from_url(url);
    if (entry->image.tex == NULL && entry->image.subtex == NULL) {
        free(entry);
        return (C2D_Image){0};
    }
    HASH_ADD_STR(imageCache, urlKey, entry);
    return entry->image;
}

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

            const char* avatarUrl = json_string_value(json_object_get(author, "avatar"));
            char* avatar_thumbnail_url = replace_substring(avatarUrl, "avatar", "avatar_thumbnail");
            data->posts[i].avatarImage = get_or_download_image(avatar_thumbnail_url);
            free(avatar_thumbnail_url);
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

void timeline_free_data(TimelinePage* data) {
    if (data == NULL) return;
    ImageCache* entry, *tmp;
    HASH_ITER(hh, imageCache, entry, tmp) {
        HASH_DEL(imageCache, entry);
        free(entry);
    }
}