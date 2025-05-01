#ifndef CLAY_RENDERER_CITRO2D_H
#define CLAY_RENDERER_CITRO2D_H

#include <citro2d.h>
#include "3ds/gfx.h"
#include "clay/clay.h"

#define GLYPH_BUFFER_SIZE 4096

#define TOP_WIDTH 400
#define TOP_HEIGHT 240

#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240

#define TOP_BOTTOM_DIFF (TOP_WIDTH - BOTTOM_WIDTH) / 2.0f

void Clay_Citro2d_Init();

u32 ClayColor_to_C2DColor(Clay_Color color);

static inline Clay_Dimensions MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void* userData) {
    Clay_Dimensions textSize = { 0 };

    float maxTextWidth = 0.0f;
    float lineTextWidth = 0;

    float textHeight = config->fontSize;
    C2D_Font* fonts = (C2D_Font*)userData;
    C2D_Font fontToUse = fonts[config->fontId];
    FINF_s* fontInfo = C2D_FontGetInfo(fontToUse);

    float scaleFactor = (float)config->fontSize/fontInfo->height;

    for (int i = 0; i < text.length; ++i) {
        if (text.chars[i] == '\n') {
            maxTextWidth = fmax(maxTextWidth, lineTextWidth);
            lineTextWidth = 0;
            continue;
        }
        int index = C2D_FontGlyphIndexFromCodePoint(fontToUse, text.chars[i]);
        charWidthInfo_s* charWidthInfo = C2D_FontGetCharWidthInfo(fontToUse, index);
        if (charWidthInfo->charWidth != 0) lineTextWidth += charWidthInfo->charWidth;
        else lineTextWidth += (charWidthInfo->glyphWidth + charWidthInfo->left);
    }

    maxTextWidth = fmax(maxTextWidth, lineTextWidth);

    textSize.width = maxTextWidth * scaleFactor;
    textSize.height = textHeight;

    return textSize;
}

void Clay_Citro2d_Render(Clay_RenderCommandArray *renderCommands, C2D_Font* fonts, gfxScreen_t screen);
void Clay_Citro2d_ClearTextCacheAndBuffer();

void Clay_Citro2d_Deinit();

#endif