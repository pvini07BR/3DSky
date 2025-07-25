#ifndef POST_H
#define POST_H

#include "thirdparty/clay/clay.h"
#include <citro2d.h>

typedef struct {
    // Can't include Feed type due to cyclic dependency
    void* feedPtr;
    C2D_Image* repliesIcon;
    C2D_Image* repostIcon;
    C2D_Image* likeIcon;

    char* uri;
    char* createdAt;
    char* indexedAt;

    unsigned int replyCount;
    unsigned int repostCount;
    unsigned int likeCount;
    unsigned int quoteCount;

    char* replyCountStr;
    char* repostCountStr;
    char* likeCountrStr;

    bool hasEmbed;

    char* displayName;
    char* handle;
    char* avatarUrl;
    char* postText;
    C2D_Image* avatarImage;
} Post;

void post_init(Post* post, void* feedPtr, C2D_Image* repliesIcon, C2D_Image* repostIcon, C2D_Image* likeIcon);
void post_layout(Post* post, bool disable, bool disableProfileLoading);
void post_update_counts(Post* post);
void post_free(Post* post);

#endif
