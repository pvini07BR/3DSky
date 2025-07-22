#include "thirdparty/clay/clay_renderer_citro2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3ds/gfx.h"
#include "3ds/gpu/enums.h"
#include "c2d/base.h"
#include "c2d/text.h"
#include "c3d/base.h"
#include "defines.h"
#include "string_utils.h"
#include "thirdparty/uthash/uthash.h"

typedef struct {
    char* key;
    C2D_Text obj;
    UT_hash_handle hh;
} TextCacheEntry;

TextCacheEntry* textCache = NULL;

static C2D_TextBuf textGlyphBuffer = NULL;

Clay_BoundingBox scissorBox;
bool scissor = false;

uint32_t hash_slice(const char *slice, size_t len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + (uint8_t)slice[i];
    }
    return hash;
}

C2D_Text* get_cached_text(C2D_Font fontToUse, const char *slice, size_t len) {
    char* key = formatted_string("%08x", hash_slice(slice, len));

    TextCacheEntry *entry = NULL;
    HASH_FIND_STR(textCache, key, entry);
    if (entry) {
        free(key);
        return &entry->obj;
    }

    entry = malloc(sizeof(TextCacheEntry));
    entry->key = strdup(key);
    free(key);

    char *temp_buf = malloc(len + 1);
    memcpy(temp_buf, slice, len);
    temp_buf[len] = '\0';

    if (textGlyphBuffer == NULL) textGlyphBuffer = C2D_TextBufNew(GLYPH_BUFFER_SIZE);

    const char* result = C2D_TextFontParse(&entry->obj, fontToUse, textGlyphBuffer, temp_buf);
    if (result) {
        if (*result != '\0') {
            fprintf(stderr, "[ERROR]: C2D_TextFontParse failed due to full glyph buffer: %s\n", temp_buf);
        }
    } else {
        fprintf(stderr, "[ERROR]: C2D_TextFontParse failed to parse text: %s\n", temp_buf);
    }

    C2D_TextOptimize(&entry->obj);

    //printf("New text object: %s\nUsed glyphs: %d\n", temp_buf, C2D_TextBufGetNumGlyphs(textGlyphBuffer));

    free(temp_buf);

    HASH_ADD_KEYPTR(hh, textCache, entry->key, strlen(entry->key), entry);

    return &entry->obj;
}

bool collides(Clay_BoundingBox* a, Clay_BoundingBox* b) {
    return (a->x < b->x + b->width &&
            a->x + a->width > b->x &&
            a->y < b->y + b->height &&
            a->y + a->height > b->y);
}

bool is_visible(gfxScreen_t screen, Clay_BoundingBox* boundingBox) {
    float x = boundingBox->x;
    float y = boundingBox->y;
    float width = boundingBox->width;
    float height = boundingBox->height;

    bool isVisibleHorizontally = false;
    bool isVisibleVertically = false;

    if (screen == GFX_TOP) {
        isVisibleHorizontally = (x + width >= 0 && x < TOP_WIDTH);
        isVisibleVertically = (y + height > 0 && y < TOP_HEIGHT);
    } else if (screen == GFX_BOTTOM) {
        isVisibleHorizontally = (x + width >= TOP_BOTTOM_DIFF &&
                                 x < BOTTOM_WIDTH + TOP_BOTTOM_DIFF);
        isVisibleVertically =
            (y + height >= TOP_HEIGHT && y < (TOP_HEIGHT + BOTTOM_HEIGHT));
    }

    return isVisibleHorizontally && isVisibleVertically;
}

void BeginScissor(gfxScreen_t screen, Clay_BoundingBox* boundingBox) {
    const int SCREEN_WIDTH = screen == GFX_TOP ? TOP_WIDTH : BOTTOM_WIDTH;
    const int SCREEN_HEIGHT = screen == GFX_TOP ? TOP_HEIGHT : BOTTOM_HEIGHT;

    int x = boundingBox->x;
    int y = boundingBox->y;
    int width = boundingBox->width;
    int height = boundingBox->height;

    C2D_Flush();

    if (is_visible(screen, boundingBox)) {
        if (screen == GFX_BOTTOM) {
            x -= TOP_BOTTOM_DIFF;
            y -= TOP_HEIGHT;
        }
    
        if ((x + width) > SCREEN_WIDTH) {
            width = SCREEN_WIDTH - x;
        }
    
        if ((y + height) > SCREEN_HEIGHT) {
            height = SCREEN_HEIGHT - y;
        }
    
        int inv_x = SCREEN_WIDTH - (x + width);
        int inv_y = SCREEN_HEIGHT - (y + height);
        
        C3D_SetScissor(GPU_SCISSOR_NORMAL,
            inv_y,            // left (Y min)
            inv_x,                // top (X min)
            inv_y + height,    // right (Y max)
            inv_x + width    // bottom (X max)
        );
    } else {
        C3D_SetScissor(
            GPU_SCISSOR_INVERT,
            0,
            0,
            SCREEN_HEIGHT,
            SCREEN_WIDTH
        );
    }

    scissorBox = *boundingBox;
    scissor = true;
}

void EndScissor() {
    C2D_Flush();
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
    scissor = false;
}

void Clay_Citro2d_Init() {
    gfxInitDefault();

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

    textGlyphBuffer = C2D_TextBufNew(GLYPH_BUFFER_SIZE);
}

u32 ClayColor_to_C2DColor(Clay_Color color) {
    return C2D_Color32(color.r, color.g, color.b, color.a);
}

void Clay_Citro2d_Render(Clay_RenderCommandArray *renderCommands, C2D_Font* fonts, gfxScreen_t screen) {
    for (int i = 0; i < renderCommands->length; i++) {
        Clay_RenderCommand* renderCommand = &renderCommands->internalArray[i];
        Clay_BoundingBox boundingBox = renderCommand->boundingBox;

        if (renderCommand->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_START && renderCommand->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_END) {
            if (!is_visible(screen, &boundingBox) || (scissor && !collides(&boundingBox, &scissorBox))) {
                continue;
            }
        }

        switch(renderCommand->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* config = &renderCommand->renderData.rectangle;

                u32 color = C2D_Color32(
                    config->backgroundColor.r,
                    config->backgroundColor.g,
                    config->backgroundColor.b,
                    config->backgroundColor.a
                );
            
                if (config->cornerRadius.topLeft > 0) {
                    float corner_radius = config->cornerRadius.topLeft;

                    float pos_x = boundingBox.x + corner_radius;
                    float pos_y = boundingBox.y + corner_radius;
                    float width = boundingBox.width - corner_radius * 2;
                    float height = boundingBox.height - corner_radius * 2;

                    C2D_DrawCircleSolid(pos_x, pos_y, 0.0f, corner_radius, color);
                    C2D_DrawCircleSolid(pos_x + width, pos_y, 0.0f, corner_radius, color);
                    C2D_DrawCircleSolid(pos_x, pos_y + height, 0.0f, corner_radius, color);
                    C2D_DrawCircleSolid(pos_x + width, pos_y + height, 0.0f, corner_radius, color);

                    // The bars
                    C2D_DrawRectSolid(pos_x, boundingBox.y, 0.0, width, corner_radius, color);
                    C2D_DrawRectSolid(pos_x + width, pos_y, 0.0, corner_radius, height, color);
                    C2D_DrawRectSolid(pos_x, pos_y + height, 0.0, width, corner_radius, color);
                    C2D_DrawRectSolid(boundingBox.x, pos_y, 0.0, corner_radius, height, color);

                    C2D_DrawRectSolid(pos_x, pos_y, 0.0, width, height, color);
                } else {
                    C2D_DrawRectSolid(
                        boundingBox.x,
                        boundingBox.y,
                        0.0,
                        boundingBox.width,
                        boundingBox.height,
                        color
                    );
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                Clay_TextRenderData* textData = &renderCommand->renderData.text;
                C2D_Font fontToUse = fonts[textData->fontId];
                FINF_s* finfo = C2D_FontGetInfo(fontToUse);

                C2D_Text *text_obj = get_cached_text(
                    fontToUse,
                    textData->stringContents.chars,
                    textData->stringContents.length
                );

                if (text_obj) {                    
                    C2D_DrawText(
                        text_obj,
                        C2D_WithColor,
                        boundingBox.x,
                        boundingBox.y,
                        0.0f,
                        (float)renderCommand->renderData.text.fontSize / (float)finfo->height,
                        (float)renderCommand->renderData.text.fontSize / (float)finfo->height,
                        C2D_Color32(
                            renderCommand->renderData.text.textColor.r,
                            renderCommand->renderData.text.textColor.g,
                            renderCommand->renderData.text.textColor.b,
                            renderCommand->renderData.text.textColor.a
                        )
                    );
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderRenderData *config = &renderCommand->renderData.border;

                // Left border
                if (config->width.left > 0) {
                    C2D_DrawRectSolid(
                        boundingBox.x,
                        boundingBox.y + config->cornerRadius.topLeft,
                        0.0, config->width.left,
                        boundingBox.height - config->cornerRadius.topLeft - config->cornerRadius.bottomLeft,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                // Right border
                if (config->width.right > 0) {
                    C2D_DrawRectSolid(
                        boundingBox.x + boundingBox.width - config->width.right,
                        boundingBox.y + config->cornerRadius.topRight,
                        0.0, config->width.right,
                        boundingBox.height - config->cornerRadius.topRight - config->cornerRadius.bottomRight,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                // Top border
                if (config->width.top > 0) {
                    C2D_DrawRectSolid(
                        boundingBox.x + config->cornerRadius.topLeft,
                        boundingBox.y,
                        0.0,
                        boundingBox.width - config->cornerRadius.topLeft - config->cornerRadius.topRight,
                        config->width.top,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                // Bottom border
                if (config->width.bottom > 0) {
                    C2D_DrawRectSolid(
                        boundingBox.x + config->cornerRadius.bottomLeft,
                        boundingBox.y + boundingBox.height - config->width.bottom,
                        0.0,
                        boundingBox.width - config->cornerRadius.bottomLeft - config->cornerRadius.bottomRight,
                        config->width.bottom,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                break;
            }
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                Clay_ImageRenderData *config = &renderCommand->renderData.image;
                C2D_Image* image = (C2D_Image*)config->imageData;

                if (image == NULL) break;
                if (image->subtex == NULL && image->tex == NULL) break;

                // Set backgroundColor on a widget with a image to change the image's color
                // do not set to make the image render as it is
                Clay_Color tintColor = renderCommand->renderData.image.backgroundColor;
                float blend = 1.0f;
                if (tintColor.r == 0 && tintColor.g == 0 && tintColor.b == 0 && tintColor.a == 0) {
                    tintColor = (Clay_Color) { 255, 255, 255, 255 };
                    blend = 0.0f;
                }
                C2D_ImageTint tint;
                C2D_PlainImageTint(&tint, ClayColor_to_C2DColor(tintColor), blend);

                float scaleX = boundingBox.width / (float)image->subtex->width;
                float scaleY = boundingBox.height / (float)image->subtex->height;

                C2D_DrawImageAt(
                    *image,
                    boundingBox.x,
                    boundingBox.y,
                    0.0f,
                    &tint,
                    scaleX,
                    scaleY
                );
            } break;
            
            // TODO: Try to implement scissor
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                BeginScissor(screen, &boundingBox);
            } break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                EndScissor();
            } break;
            default:
                break;
        }
    }
}

void Clay_Citro2d_ClearTextCacheAndBuffer() {
    TextCacheEntry *entry, *tmp;
    HASH_ITER(hh, textCache, entry, tmp) {
        HASH_DEL(textCache, entry);
        free(entry->key);
        free(entry);
    }
    if (textGlyphBuffer) {
        C2D_TextBufClear(textGlyphBuffer);
    }
}

void Clay_Citro2d_Deinit() {
    Clay_Citro2d_ClearTextCacheAndBuffer();

    if (textGlyphBuffer != NULL) {
        C2D_TextBufDelete(textGlyphBuffer);
    }
    C2D_Fini();
    C3D_Fini();
    gfxExit();

}
