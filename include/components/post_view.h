#ifndef POST_VIEW_H
#define POST_VIEW_H

#include "components/post.h"

typedef struct {
    Post* post;
    bool opened;

    float postViewScroll;
} PostView;

void post_view_init(PostView* data);
void post_view_set(PostView* data, Post* post);
void post_view_layout(PostView* data);
void post_view_input(PostView* data, float deltaTime);

#endif