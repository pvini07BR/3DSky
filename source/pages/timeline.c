#include <3ds.h>
#include "3ds/thread.h"
#include "components/feed.h"
#include "thirdparty/bluesky/bluesky.h"
#include "pages/timeline.h"

bool stopAvatarThread = false;
bool stopPostLoadingThread = false;

Thread postLoadingThreadHnd = NULL;
Thread avatarThreadHnd = NULL;

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

void loadPostsThread(void* args) {
    if (args == NULL) return;
    
    TimelinePage* data = (TimelinePage*)args;
    if (data == NULL) return;

    feed_load_timeline(&data->feed);
}

void timeline_init(TimelinePage* data) {
    if (data == NULL){return;}
    postLoadingThreadHnd = threadCreate(loadPostsThread, data, (16 * 1024), 0x3f, -2, true);
    data->initialized = true;
}

void timeline_page_layout(TimelinePage *data) {
    if (data == NULL) { return; }

    feed_layout(&data->feed);
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

bool timeline_has_loaded_posts(TimelinePage* data) {
    if (data == NULL) return false;
    return data->feed.loaded;
}