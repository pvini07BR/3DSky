#ifndef FEED_H
#define FEED_H

#include "components/post.h"
#include "components/post_view.h"
#include "thirdparty/bluesky/bluesky.h"
#include "thread_pool.h"

typedef enum {
    FEED_TYPE_TIMELINE,
    FEED_TYPE_AUTHOR
} FeedType;

typedef struct {
    bs_client_pagination_opts pagOpts;
    Post posts[50];
    bool loaded;
    bool disableProfileLoading;
    FeedType type;
    PostView* postViewPtr;
    float scrollValue;
    float prevScroll;
    bool scrolling;
    bool setScroll;
    ThreadTaskHandle loadingThreadHandle;
    //ThreadTaskHandle avatarThreadHandle;
    bool stopLoadingThread;
    bool stopAvatarThread;

    // This member variable will only be used for author posts
    char* did;
} Feed;

void feed_init(Feed* feed, FeedType feed_type, PostView* postViewPtr, bool disableProfileLoading, C2D_Image* repliesIcon, C2D_Image* repostIcon, C2D_Image* likeIcon);
void feed_load(Feed* feed, bool resetCursor);
void feed_layout(Feed* feed, float top_padding);
void feed_stop_threads(Feed* feed);
void feed_free(Feed* feed);

#endif