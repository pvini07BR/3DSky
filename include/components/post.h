#ifndef POST_H
#define POST_H

#include "thirdparty/clay/clay.h"
#include <citro2d.h>

typedef struct {
    char* displayName;
    char* handle;
    char* postText;
    char* avatarUrl;
    C2D_Image* avatarImage;
} Post;

void post_component(Post* post, void (*post_open_callback)(void*, Post*), void* context);
void post_free(Post* post);

#endif
