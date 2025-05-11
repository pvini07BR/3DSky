#ifndef PROFILE_PAGE_H
#define PROFILE_PAGE_H

#include <citro2d.h>

typedef struct {
    bool initialized;

    C2D_Image* avatarImage;
    const char* displayName;
    const char* handle;
    const char* description;

    unsigned int followersCount;
    unsigned int followsCount;
    unsigned int postsCount;
} ProfilePage;

void profile_page_load(ProfilePage* data, const char* handle);
void profile_page_layout(ProfilePage* data);
void profile_page_free();

#endif
