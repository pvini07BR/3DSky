#include <3ds.h>
#include "components/feed.h"
#include "defines.h"
#include "thirdparty/bluesky/bluesky.h"
#include "pages/timeline.h"

//bool stopAvatarThread = false;
//bool stopPostLoadingThread = false;

//Thread avatarThreadHnd = NULL;

// TODO: Try to somehow make all posts using the same avatar URL load instantly after downloading the image,
// and then go to download the next one.
/*
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
*/

void timeline_init(TimelinePage* data) {
    if (data == NULL){return;}
    feed_load_timeline(&data->feed);
    data->initialized = true;
}

void timeline_page_layout(TimelinePage *data) {
    if (data == NULL) { return; }

    CLAY({
        .layout = {
            .sizing = { CLAY_SIZING_FIXED(TOP_BOTTOM_DIFF + BOTTOM_WIDTH + 1), CLAY_SIZING_GROW() },
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .border = {
            .width = {
                .right = 1
            },
            .color = {46, 64, 82, 255}
        }
    }) {
        // Empty space to push the posts to fit in the bottom screen
        CLAY({
            .layout = {
                .sizing = { CLAY_SIZING_FIXED(TOP_BOTTOM_DIFF), CLAY_SIZING_GROW() }
            },
            .border = {
                .width = {
                    .right = 1
                },
                .color = {46, 64, 82, 255}
            }
        });

        // Where the posts will be
        feed_layout(&data->feed, TOP_HEIGHT);
    }
}

void timeline_free(TimelinePage* data) {
    feed_free(&data->feed);
}