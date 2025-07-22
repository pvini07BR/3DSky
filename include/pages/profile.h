#ifndef PROFILE_PAGE_H
#define PROFILE_PAGE_H

#include "components/feed.h"
#include <citro2d.h>
#include "thirdparty/bluesky/bluesky.h"

typedef struct {
    const char* handle;

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

    Thread loadingThreadHandle;

} ProfilePage;

void profile_page_init(ProfilePage* data);
void profile_page_load(ProfilePage* data, const char* handle);
void profile_page_layout(ProfilePage* data, float deltaTime);
void profile_page_free(ProfilePage* data);

#endif
