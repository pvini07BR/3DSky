#ifndef POST_H
#define POST_H

#include "thirdparty/clay/clay.h"
#include <citro2d.h>

struct Post {
    const char* displayName;
    const char* handle;
    const char* postText;
    const char* avatarUrl;
    C2D_Image* avatarImage;
};

void post_component(struct Post* post);

#endif
