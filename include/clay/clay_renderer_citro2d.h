// clay_renderer_citro2d.h
#ifndef CLAY_RENDERER_CITRO2D_H
#define CLAY_RENDERER_CITRO2D_H

#include <citro2d.h>
#include "clay/clay.h"

#define GLYPH_BUFFER_SIZE   4096
#define STRING_SLOT_COUNT   16
#define STRING_SLOT_SIZE    256

#define TOP_WIDTH 400
#define TOP_HEIGHT 240

#define BOTTOM_WIDTH 320
#define BOTTOM_HEIGHT 240

#define TOP_BOTTOM_DIFF (TOP_WIDTH - BOTTOM_WIDTH) / 2.0f

// Initialize Citro2D + text buffer
void Clay_Citro2d_Init(void);

// Convert Clay_Color to Citro2D's u32 color
u32  ClayColor_to_C2DColor(Clay_Color color);

// Measure a block of text (width/height) using C2D font info.
// userData should be the C2D_Font* array you pass into Render.
static inline Clay_Dimensions MeasureText(Clay_StringSlice        text,
                                           Clay_TextElementConfig *config,
                                           void                   *userData)
{
    Clay_Dimensions size = { 0 };
    float          maxW = 0, lineW = 0;
    float          fontH = config->fontSize;

    C2D_Font* fonts    = (C2D_Font*)userData;
    C2D_Font  font     = fonts[config->fontId];
    FINF_s*   finfo    = C2D_FontGetInfo(font);
    float     scale    = config->fontSize / (float)finfo->height;

    for (int i = 0; i < text.length; i++) {
        char c = text.chars[i];
        if (c == '\n') {
            maxW   = fmaxf(maxW, lineW);
            lineW  = 0;
            continue;
        }
        int idx = C2D_FontGlyphIndexFromCodePoint(font, c);
        charWidthInfo_s* winfo = C2D_FontGetCharWidthInfo(font, idx);
        lineW += winfo->charWidth
               ? winfo->charWidth
               : (winfo->glyphWidth + winfo->left);
    }
    maxW = fmaxf(maxW, lineW);
    size.width  = maxW * scale;
    size.height = fontH;
    return size;
}

// Render loop: rectangles, text, (stub) clipping, etc.
// The screen argument is used to determine if the element is visible on the current screen.
void Clay_Citro2d_Render(Clay_RenderCommandArray *renderCommands,
                         C2D_Font*                fonts,
                         gfxScreen_t              screen);

// Clean up everything
void Clay_Citro2d_Deinit(void);

#endif // CLAY_RENDERER_CITRO2D_H