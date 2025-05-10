#include "avatar_img_cache.h"

#include <stdlib.h>
#include "uthash/uthash.h"
#include "image_downloader.h"

typedef struct {
    char* urlKey;
    C2D_Image avatarImage;
    UT_hash_handle hh;
} AvatarImgCache;

AvatarImgCache* imageCache = NULL;

C2D_Image* avatar_img_cache_get_or_download_image(const char* url) {
    if (url == NULL) {
        return NULL;
    }

    AvatarImgCache* entry = NULL;
    HASH_FIND_STR(imageCache, url, entry);
    if (entry) return &entry->avatarImage;

    entry = (AvatarImgCache*)malloc(sizeof(AvatarImgCache));
    if (!entry) {
        perror("Error allocating memory for image cache entry");
        return NULL;
    }
    entry->urlKey = malloc(strlen(url) + 1);
    if (!entry->urlKey) {
        perror("Error allocating memory for URL key");
        free(entry);
        return NULL;
    }
    strcpy(entry->urlKey, url);
    entry->urlKey[strlen(url)] = '\0';
    entry->avatarImage = download_image_from_url(url, 32, 32);
    if (entry->avatarImage.tex == NULL && entry->avatarImage.subtex == NULL) {
        free(entry);
        return NULL;
    }
    HASH_ADD_STR(imageCache, urlKey, entry);
    return &entry->avatarImage;
}

void avatar_img_cache_free() {
    AvatarImgCache* entry, *tmp;
    HASH_ITER(hh, imageCache, entry, tmp) {
        HASH_DEL(imageCache, entry);
        free(entry);
    }
}
