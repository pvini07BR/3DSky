#ifndef AVATAR_IMG_CACHE_H
#define AVATAR_IMG_CACHE_H

#include <citro2d.h>

C2D_Image* avatar_img_cache_get_or_download_image(const char* url, unsigned int width, unsigned int height);
void avatar_img_cache_free();

#endif
