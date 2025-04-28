#ifndef TIMELINE_PAGE_H
#define TIMELINE_PAGE_H

#include "components/post.h"

typedef struct {
    const char* cursor;
    struct Post posts[50];
    bool postsLoaded;
} TimelinePage;

void timeline_page_load_posts(TimelinePage* data);
void timeline_page_layout(TimelinePage* data);
void timeline_free_data(TimelinePage* data);

#endif