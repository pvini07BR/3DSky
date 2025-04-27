#include "image_downloader.h"
#include "c3d/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <sys/select.h>
#include <curl/curl.h>

typedef struct {
	void *memory;
	size_t byte_size;
} ImageMemory;

u32 next_pow2(u32 i)
{
	--i;
	i |= i >> 1;
	i |= i >> 2;
	i |= i >> 4;
	i |= i >> 8;
	i |= i >> 16;
	++i;

	return i;
}

size_t write_data_placeholder(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

size_t img_mem_write_callback(void *buffer, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	ImageMemory *mem = (ImageMemory *)userp;

	memcpy(mem->memory + mem->byte_size, buffer, realsize);
	mem->byte_size += realsize;

	return realsize;
}

C2D_Image download_image_from_url(const char* url) {
    // The C2D_Image is a struct that is comprised only of pointers.
    // Therefore, it's possible to check if the image has been loaded properly by checking if they're NULL.
    C2D_Image image = {0};

    if (url == NULL || strlen(url) == 0) { return image; }

    CURL *hnd = curl_easy_init();
    if (hnd == NULL) { return image; }

    // First try to reach the URL
    curl_easy_setopt(hnd, CURLOPT_URL, url);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, write_data_placeholder);

    CURLcode ret = curl_easy_perform(hnd);

    if (ret != CURLE_OK) {
        curl_easy_cleanup(hnd);
        return image;
    }

    // Now get the download size to properly allocate the memory for the image.
    double download_size;
    ret = curl_easy_getinfo(hnd, CURLINFO_SIZE_DOWNLOAD, &download_size);

    ImageMemory imgmem = {0};
    imgmem.memory = (stbi_uc*)malloc(download_size);
    imgmem.byte_size = 0;

    // And now we start to download the image.
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, img_mem_write_callback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &imgmem);

    ret = curl_easy_perform(hnd);
    if (ret != CURLE_OK) {
        free(imgmem.memory);
        curl_easy_cleanup(hnd);
        return image;
    }

    // Load the downloaded image data into stbi
    int img_width, img_height, n_channels;
    unsigned char *stbi_img = stbi_load_from_memory(
        (stbi_uc*)imgmem.memory,
        imgmem.byte_size,
        &img_width,
        &img_height,
        &n_channels,
        4
    );
    if (stbi_img == NULL) {
        fprintf(stderr, "Failed to load image: %s\n", stbi_failure_reason());
        free(imgmem.memory);
        curl_easy_cleanup(hnd);
        return image;
    }

    // The size of images on the 3DS needs to be powers of 2.
    uint32_t wtex = next_pow2(img_width);
    uint32_t htex = next_pow2(img_height);

    // Now create the texture object
    image.tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    Tex3DS_SubTexture* newSubtex = (Tex3DS_SubTexture*)malloc(sizeof(Tex3DS_SubTexture));
    newSubtex->width = (u16)img_width;
    newSubtex->height = (u16)img_height;
    newSubtex->left = 0.0f;
    newSubtex->top = 1.0f;
    newSubtex->right = img_width / (float)wtex;
    newSubtex->bottom = 1.0f - (img_height / (float)htex);

    image.subtex = newSubtex;

    // Initialize it
    if (!C3D_TexInit(image.tex, wtex, htex, GPU_RGBA8)) {
        free(image.tex);
        free(newSubtex);
        image.tex = NULL;
        image.subtex = NULL;
        return image;
    }

    // Set filters and wrap modes
    C3D_TexSetFilter(image.tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(image.tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
    image.tex->border = 0x00FFFFFF;

    // And finally copy the image data to the texture
    for(u32 y = 0; y < img_height; y++) {
        for(u32 x = 0; x < img_width; x++) {
            const u32 dst_pixel = ((((y >> 3) * (wtex >> 3) + (x >> 3)) << 6) +
                                ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
                                ((x & 4) << 2) | ((y & 4) << 3))) * 4;

            const u32 src_pixel = (y * (img_width * 4)) + (x * 4);

            // Images on the 3DS are in ABGR (i think)
            ((uint8_t*)image.tex->data)[dst_pixel + 0] = stbi_img[src_pixel + 3];
            ((uint8_t*)image.tex->data)[dst_pixel + 1] = stbi_img[src_pixel + 2];
            ((uint8_t*)image.tex->data)[dst_pixel + 2] = stbi_img[src_pixel + 1];
            ((uint8_t*)image.tex->data)[dst_pixel + 3] = stbi_img[src_pixel + 0];
        }
    }

    stbi_image_free(stbi_img);
    free(imgmem.memory);
    curl_easy_cleanup(hnd);
    return image;
}