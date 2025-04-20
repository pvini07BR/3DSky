#ifndef POST_H
#define POST_H

#include "clay/clay.h"

struct Post {
    const char* displayName;
    const char* handle;
    const char* postText;
};

void post_component(struct Post* post);

#endif
