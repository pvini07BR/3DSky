#ifndef TIMELINE_PAGE_H
#define TIMELINE_PAGE_H

#include "components/feed.h"
#include "components/post_view.h"
#include "thirdparty/bluesky/bluesky.h"

typedef struct {
    bool initialized;
    Feed feed;
    PostView postView;
} TimelinePage;

void timeline_init(TimelinePage* data, C2D_Image* repliesIcon, C2D_Image* repostIcon, C2D_Image* likeIcon);
void timeline_update(TimelinePage* data, float deltaTime);
void timeline_page_layout(TimelinePage* data, float deltaTime);
void timeline_free(TimelinePage* data);

#endif
