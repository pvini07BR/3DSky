#ifndef IMAGE_DOWNLOADER_H
#define IMAGE_DOWNLOADER_H

#include <citro2d.h>

C2D_Image download_image_from_url(const char* url, unsigned int custom_wdith, unsigned int custom_height);

#endif