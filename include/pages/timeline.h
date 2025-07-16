#ifndef TIMELINE_PAGE_H
#define TIMELINE_PAGE_H

#include "components/feed.h"
#include "thirdparty/bluesky/bluesky.h"

typedef struct {
    bool initialized;
    Feed feed;
} TimelinePage;

void timeline_init(TimelinePage* data);
void timeline_page_layout(TimelinePage* data);
void timeline_free(TimelinePage* data);

#endif
