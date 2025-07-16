#ifndef FEED_H
#define FEED_H

#include "components/post.h"
#include "jansson.h"
#include "thirdparty/bluesky/bluesky.h"

typedef enum {
    FEED_TYPE_TIMELINE,
    FEED_TYPE_AUTHOR
} FeedType;

typedef struct {
    bs_client_pagination_opts pagOpts;
    Post posts[50];
    bool loaded;
    FeedType type;
    float scrollValue;
    bool setScroll;
} Feed;

void feed_load_timeline(Feed* feed);
void feed_layout(Feed* feed);

#endif