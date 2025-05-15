#ifndef TIMELINE_PAGE_H
#define TIMELINE_PAGE_H

#include "components/post.h"

typedef struct {
    bool initialized;
    const char* cursor;
    struct Post posts[50];
    bool postsLoaded;
    float scrollValue;
    bool setScroll;
} TimelinePage;

void timeline_page_load_posts(TimelinePage* data);
void timeline_page_layout(TimelinePage* data);
void timeline_free_data(TimelinePage* data);
void timeline_stop_avatar_thread();

#endif
