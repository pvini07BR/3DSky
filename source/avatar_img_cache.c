#include "avatar_img_cache.h"

#include <stdlib.h>
#include "3ds/synchronization.h"
#include "thirdparty/uthash/uthash.h"
#include "image_downloader.h"

typedef struct {
    const char* urlKey;
    C2D_Image avatarImage;
    UT_hash_handle hh;
} AvatarImgCache;

AvatarImgCache* imageCache = NULL;

LightLock cacheLock;

void avatar_img_cache_init() {
    LightLock_Init(&cacheLock);
}

C2D_Image* avatar_img_cache_get_or_download_image(const char* url, unsigned int width, unsigned int height) {
    if (url == NULL) return NULL;

    AvatarImgCache* entry = NULL;
    HASH_FIND_STR(imageCache, url, entry);
    if (entry) return &entry->avatarImage;
    
    LightLock_Lock(&cacheLock);
    entry = (AvatarImgCache*)malloc(sizeof(AvatarImgCache));
    if (!entry) {
        perror("Error allocating memory for image cache entry");
        return NULL;
    }

    entry->urlKey = url;
    entry->avatarImage = download_image_from_url(url, width, height);
    if (entry->avatarImage.tex == NULL || entry->avatarImage.subtex == NULL) {
        free(entry);
        return NULL;
    }

    HASH_ADD_KEYPTR(hh, imageCache, entry->urlKey, strlen(entry->urlKey), entry);
    LightLock_Unlock(&cacheLock);
    return &entry->avatarImage;
}

void avatar_img_cache_free() {
    AvatarImgCache* entry, *tmp;
    HASH_ITER(hh, imageCache, entry, tmp) {
        HASH_DEL(imageCache, entry);
        free(entry);
    }
}
