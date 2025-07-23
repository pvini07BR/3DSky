#ifndef PROFILE_PAGE_H
#define PROFILE_PAGE_H

#include "components/feed.h"
#include <citro2d.h>
#include "thirdparty/bluesky/bluesky.h"
#include "thread_pool.h"

typedef struct {
    char* handle;

    bool initialized;
    bool loaded;
    
    C2D_Image* avatarImage;
    char* displayName;
    char* description;
    char* followsText;

    unsigned int followersCount;
    unsigned int followsCount;
    unsigned int postsCount;

    Feed feed;

    ThreadTaskHandle loadingThreadHandle;
} ProfilePage;

void profile_page_init(ProfilePage* data, const char* handle, C2D_Image* repliesIcon, C2D_Image* repostIcon, C2D_Image* likeIcon);
void profile_page_load(ProfilePage* data, const char* handle);
void profile_page_layout(ProfilePage* data, float deltaTime);
void profile_page_free(ProfilePage* data);

#endif
