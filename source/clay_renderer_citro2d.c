#include "clay/clay_renderer_citro2d.h"
#include <stdlib.h>
#include <string.h>

#include "uthash/uthash.h"

typedef struct {
    char* text;
    C2D_Text obj;
    UT_hash_handle hh;
} TextCache;

TextCache* textCache = NULL;

static C2D_TextBuf textBuf = NULL;
static char *temp_render_buffer = NULL;
static int temp_render_buffer_len = 0;

bool is_visible(gfxScreen_t screen, Clay_BoundingBox boundingBox) {
    float x = boundingBox.x;
    float y = boundingBox.y;
    float width = boundingBox.width;
    float height = boundingBox.height;

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

void Clay_Citro2d_Init() {
    gfxInitDefault();

	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

    textBuf = C2D_TextBufNew(GLYPH_BUFFER_SIZE);
}

u32 ClayColor_to_C2DColor(Clay_Color color) {
    return C2D_Color32(color.r, color.g, color.b, color.a);
}

void Clay_Citro2d_Render(Clay_RenderCommandArray *renderCommands, C2D_Font* fonts, gfxScreen_t screen) {
    for (int i = 0; i < renderCommands->length; i++) {
        Clay_RenderCommand* renderCommand = &renderCommands->internalArray[i];
        Clay_BoundingBox boundingBox = renderCommand->boundingBox;

        if (!is_visible(screen, boundingBox)) {
            continue;
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
                
                int slen = textData->stringContents.length + 1;
                
                if(slen > temp_render_buffer_len) {
                    if(temp_render_buffer) free(temp_render_buffer);
                    temp_render_buffer = (char*)malloc(slen);
                    temp_render_buffer_len = slen;
                }
                memcpy(temp_render_buffer, textData->stringContents.chars, textData->stringContents.length);
                temp_render_buffer[textData->stringContents.length] = '\0';

                TextCache* entry = NULL;
                HASH_FIND_STR(textCache, temp_render_buffer, entry);
                if (entry == NULL && textBuf != NULL) {
                    entry = (TextCache*)malloc(sizeof(TextCache));
                    entry->text = malloc(slen);
                    memcpy(entry->text, temp_render_buffer, slen);

                    if (C2D_TextBufGetNumGlyphs(textBuf) >= GLYPH_BUFFER_SIZE) {
                        textBuf = C2D_TextBufResize(textBuf, C2D_TextBufGetNumGlyphs(textBuf) + slen);
                    }
                    
                    C2D_TextFontParse(
                        &entry->obj,
                        fontToUse,
                        textBuf,
                        temp_render_buffer
                    );
                    C2D_TextOptimize(&entry->obj);
                    
                    HASH_ADD_STR(textCache, text, entry);
                }

                if (entry == NULL) {
                    break;
                }

                C2D_DrawText(
                    &entry->obj,
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
                break;
            }
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
                
                float scale = (scaleX < scaleY) ? scaleX : scaleY;
                
                float scaledWidth = (float)image->subtex->width * scale;
                float scaledHeight = (float)image->subtex->height * scale;
                
                float x = boundingBox.x + (boundingBox.width - scaledWidth) / 2.0f;
                float y = boundingBox.y + (boundingBox.height - scaledHeight) / 2.0f;

                C2D_DrawImageAt(
                    *image,
                    x,
                    y,
                    0.0f,
                    &tint,
                    scale,
                    scale
                );
            } break;
            default:
                break;
        }
    }
}

void Clay_Citro2d_ClearTextCacheAndBuffer() {
    TextCache *currentCache, *tmpCache;
    HASH_ITER(hh, textCache, currentCache, tmpCache) {
        HASH_DEL(textCache, currentCache);
        free(currentCache->text);
        free(currentCache);
    }
    if (textBuf != NULL) {
        C2D_TextBufClear(textBuf);
    }
}

void Clay_Citro2d_Deinit() {
    TextCache *currentCache, *tmpCache;

    HASH_ITER(hh, textCache, currentCache, tmpCache) {
        HASH_DEL(textCache, currentCache);
        free(currentCache->text);
        free(currentCache);
    }

    if(temp_render_buffer) free(temp_render_buffer);

    if (textBuf != NULL) {
        C2D_TextBufDelete(textBuf);
    }
    C2D_Fini();
    C3D_Fini();
    gfxExit();

}
