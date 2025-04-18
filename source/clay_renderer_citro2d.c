// clay_renderer_citro2d.c
#include "clay/clay_renderer_citro2d.h"
#include <string.h>
#include <stdio.h>

static C2D_TextBuf textBuf = NULL;

// Ring‑buffer for temporary c‑strings
static char string_slots[STRING_SLOT_COUNT][STRING_SLOT_SIZE];
static int  current_slot = 0;

// Allocates one slot, copies and NUL‑terminates.
// Safe for up to STRING_SLOT_COUNT simultaneous strings per frame.
static const char* AllocStringSlot(const Clay_StringSlice* str) {
    current_slot = (current_slot + 1) % STRING_SLOT_COUNT;
    size_t len = str->length;
    if (len >= STRING_SLOT_SIZE) len = STRING_SLOT_SIZE - 1;
    memcpy(string_slots[current_slot], str->chars, len);
    string_slots[current_slot][len] = '\0';
    return string_slots[current_slot];
}

void Clay_Citro2d_Init(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    textBuf = C2D_TextBufNew(GLYPH_BUFFER_SIZE);
}

u32 ClayColor_to_C2DColor(Clay_Color color) {
    return C2D_Color32(color.r, color.g, color.b, color.a);
}

void Clay_Citro2d_Render(Clay_RenderCommandArray *renderCommands,
                         C2D_Font*                fonts)
{
    // Prevent any mid‐frame glyph‐buffer flush/flicker
    if (textBuf) C2D_TextBufClear(textBuf);

    for (int i = 0; i < renderCommands->length; i++) {
        Clay_RenderCommand* cmd = &renderCommands->internalArray[i];
        Clay_BoundingBox     bb  = cmd->boundingBox;

        switch (cmd->commandType) {

            // ---- RECTANGLE (with optional rounded corners) ----
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                Clay_RectangleRenderData* cfg = &cmd->renderData.rectangle;
                u32 color = ClayColor_to_C2DColor(cfg->backgroundColor);

                if (cfg->cornerRadius.topLeft > 0.0f) {
                    float r = cfg->cornerRadius.topLeft;
                    float x = bb.x + r, y = bb.y + r;
                    float w = bb.width  - 2*r;
                    float h = bb.height - 2*r;
                    // corners
                    C2D_DrawCircleSolid(x,     y,     0.0f, r, color);
                    C2D_DrawCircleSolid(x + w, y,     0.0f, r, color);
                    C2D_DrawCircleSolid(x,     y + h, 0.0f, r, color);
                    C2D_DrawCircleSolid(x + w, y + h, 0.0f, r, color);
                    // edges + fill
                    C2D_DrawRectSolid(x,     bb.y,  0.0f, w, r, color);
                    C2D_DrawRectSolid(x + w, y,     0.0f, r, h, color);
                    C2D_DrawRectSolid(x,     y + h, 0.0f, w, r, color);
                    C2D_DrawRectSolid(bb.x,  y,     0.0f, r, h, color);
                    C2D_DrawRectSolid(x,     y,     0.0f, w, h, color);
                } else {
                    C2D_DrawRectSolid(bb.x, bb.y, 0.0f, bb.width, bb.height,
                                      color);
                }
            } break;

            // ---- TEXT ----
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                if (!textBuf) break;
                Clay_TextRenderData* td   = &cmd->renderData.text;
                C2D_Font            font = fonts[td->fontId];
                const char*         txt  = AllocStringSlot(&td->stringContents);
                FINF_s* finfo = C2D_FontGetInfo(font);
                float scale = td->fontSize / (float)finfo->height;

                C2D_Text textObj;
                C2D_TextFontParse(&textObj, font, textBuf, txt);
                C2D_TextOptimize(&textObj);
                C2D_DrawText(&textObj,
                             C2D_WithColor,
                             bb.x, bb.y, 0.0f,
                             scale, scale,
                             ClayColor_to_C2DColor(td->textColor));

                #ifdef CLAY_DEBUG_TEXT
                int used = C2D_TextBufGetNumGlyphs(textBuf);
                if (used >= GLYPH_BUFFER_SIZE - 64) {
                    printf("[Clay][WARN] Glyph buf %d/%d\n",
                           used, GLYPH_BUFFER_SIZE);
                }
                #endif
            } break;
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                Clay_BorderRenderData *config = &cmd->renderData.border;

                // Left border
                if (config->width.left > 0) {
                    C2D_DrawRectSolid(
                        bb.x,
                        bb.y + config->cornerRadius.topLeft,
                        0.0, config->width.left,
                        bb.height - config->cornerRadius.topLeft - config->cornerRadius.bottomLeft,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                // Right border
                if (config->width.right > 0) {
                    C2D_DrawRectSolid(
                        bb.x + bb.width - config->width.right,
                        bb.y + config->cornerRadius.topRight,
                        0.0, config->width.right,
                        bb.height - config->cornerRadius.topRight - config->cornerRadius.bottomRight,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                // Top border
                if (config->width.top > 0) {
                    C2D_DrawRectSolid(
                        bb.x + config->cornerRadius.topLeft,
                        bb.y,
                        0.0,
                        bb.width - config->cornerRadius.topLeft - config->cornerRadius.topRight,
                        config->width.top,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
                // Bottom border
                if (config->width.bottom > 0) {
                    C2D_DrawRectSolid(
                        bb.x + config->cornerRadius.bottomLeft,
                        bb.y + bb.height - config->width.bottom,
                        0.0,
                        bb.width - config->cornerRadius.bottomLeft - config->cornerRadius.bottomRight,
                        config->width.bottom,
                        C2D_Color32(config->color.r, config->color.g, config->color.b, config->color.a)
                    );
                }
            } break;
            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                Clay_ImageRenderData *config = &cmd->renderData.image;
                C2D_Image image = *(C2D_Image*)config->imageData;

                // Set backgroundColor on a widget with a image to change the image's color
                // do not set to make the image render as it is
                Clay_Color tintColor = cmd->renderData.image.backgroundColor;
                float blend = 1.0f;
                if (tintColor.r == 0 && tintColor.g == 0 && tintColor.b == 0 && tintColor.a == 0) {
                    tintColor = (Clay_Color) { 255, 255, 255, 255 };
                    blend = 0.0f;
                }
                C2D_ImageTint tint;
                C2D_PlainImageTint(&tint, ClayColor_to_C2DColor(tintColor), blend);
                
                float scaleX = bb.width / (float)image.subtex->width;
                float scaleY = bb.height / (float)image.subtex->height;
                
                // Usar a menor escala para manter a proporção da imagem
                float scale = (scaleX < scaleY) ? scaleX : scaleY;
                
                // Calcular as dimensões da imagem escalada
                float scaledWidth = (float)image.subtex->width * scale;
                float scaledHeight = (float)image.subtex->height * scale;
                
                // Calcular a posição para centralizar a imagem
                float x = bb.x + (bb.width - scaledWidth) / 2.0f;
                float y = bb.y + (bb.height - scaledHeight) / 2.0f;

                C2D_DrawImageAt(
                    image,
                    x,
                    y,
                    0.0f,
                    &tint,
                    scale,
                    scale
                );
            } break;

            // ---- (future) CLIP ----
            //case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                // Clay_ClipRenderData* cd = &cmd->renderData.clip;
                // if (cd->enable) {
                //     C2D_TargetSetScissor(0, cd->x, cd->y,
                //                         cd->width, cd->height);
                // } else {
                //     C2D_TargetResetScissor(0);
                // }
            //} break;

            default:
                break;
        }
    }
}

void Clay_Citro2d_Deinit(void) {
    if (textBuf) C2D_TextBufDelete(textBuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}