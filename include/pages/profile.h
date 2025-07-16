#ifndef PROFILE_PAGE_H
#define PROFILE_PAGE_H

#include "components/feed.h"
#include <citro2d.h>
#include "thirdparty/bluesky/bluesky.h"

typedef struct {
    bool initialized;
    bool loaded;

    C2D_Image* avatarImage;
    const char* displayName;
    const char* handle;
    const char* description;

    unsigned int followersCount;
    unsigned int followsCount;
    unsigned int postsCount;

    Feed feed;
} ProfilePage;

void profile_page_load(ProfilePage* data, const char* handle);
void profile_page_layout(ProfilePage* data);
void profile_page_free();

#endif
