#ifndef POST_H
#define POST_H

#include "thirdparty/clay/clay.h"
#include <citro2d.h>
#include <stdint.h>

typedef struct {
    // Can't include Feed type due to cyclic dependency
    void* feedPtr;

    char* displayName;
    char* handle;
    char* postText;
    char* avatarUrl;
    C2D_Image* avatarImage;
} Post;

void post_component(Post* post, void (*onHoverFunction)(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData), bool disable);
void post_free(Post* post);

#endif
