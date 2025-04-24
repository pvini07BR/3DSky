#include "3ds/types.h"
#include <malloc.h>
#include <stdio.h>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "clay/clay_renderer_citro2d.h"

#include "bluesky/bluesky.h"

#include "scenes/login_scene.h"
#include "scenes/main_scene.h"

enum ConsoleMode {
    OFF,
    TOP,
    BOTTOM
};

enum ConsoleMode consoleMode = OFF;

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s\n", errorData.errorText.chars);
}

int main() {
    romfsInit();

    Result ret = 0;
    u32 soc_sharedmem_size = 0x100000;
    u32 *soc_sharedmem = (u32*)memalign(0x1000, soc_sharedmem_size);
    if (soc_sharedmem != NULL) {
        ret = socInit(soc_sharedmem, soc_sharedmem_size);

        if (!R_FAILED(ret)) {
            printf("SOC succesfully initialized.\n");
        } else {
            printf("socInit failed: 0x%08x.\n", (unsigned int)ret);
            return -1;
        }
    } else {
        printf("Failed to allocate SOC sharedmem.\n");
        return -1;
    }

	Clay_Citro2d_Init();

    C3D_RenderTarget* top = NULL;
    C3D_RenderTarget* bottom = NULL;

    if (consoleMode == TOP) {
        consoleInit(GFX_TOP, NULL);
        bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    } else if (consoleMode == BOTTOM) {
        consoleInit(GFX_BOTTOM, NULL);
        top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    } else {
        top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
        bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    }

    C2D_Font fonts[2];
    fonts[0] = C2D_FontLoadSystem(CFG_REGION_USA);

    C2D_FontSetFilter(fonts[0], GPU_LINEAR, GPU_LINEAR);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions) { TOP_WIDTH, TOP_HEIGHT + BOTTOM_HEIGHT }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(MeasureText, &fonts);

    // Use this code to login if you don't want to get through the login scene over and over
    //bs_client_init("user.bsky.social", "password", NULL);

    // Change get_login_scene() to get_main_scene() to skip the login scene
    change_scene(get_login_scene());

    touchPosition tempPos = {-1};
    Clay_Vector2 lastTouchPos = {-1.0f, -1.0f};

    u64 lastTime = osGetTime();
    while(aptMainLoop()) {
        hidScanInput();
        
		u32 kDown = hidKeysDown();
        
		if (kDown & KEY_START)
        break;
    
        touchPosition touch = {-1};
        hidTouchRead(&touch);
        
        if (touch.px != 0 && touch.py != 0) {
            tempPos = touch;
        }

        u64 currentTime = osGetTime();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        float touch_x = tempPos.px + TOP_BOTTOM_DIFF;
        float touch_y = tempPos.py + TOP_HEIGHT;

        Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, touch.px != 0 && touch.py != 0);
        if (touch.px != 0 && touch.py != 0) {
            if (lastTouchPos.x != -1.0f && lastTouchPos.y != -1.0f) {
                Clay_Vector2 scrollDelta = (Clay_Vector2) {touch_x - lastTouchPos.x, touch_y - lastTouchPos.y};
                Clay_UpdateScrollContainers(true, scrollDelta, deltaTime);                
            } else {
                Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, 0}, deltaTime);
            }
            lastTouchPos = (Clay_Vector2) {touch_x, touch_y};
        } else {
            lastTouchPos = (Clay_Vector2) {-1.0f, -1.0f};
            Clay_UpdateScrollContainers(true, (Clay_Vector2){0.0f, 0.0f}, deltaTime);
        }

        Scene* current = get_current_scene();
        if (current != NULL) current->update();

        Clay_BeginLayout();

        if (current != NULL) current->layout();

        // I was trying to do FPS on here but the text gets glitched for some reason
        /*
        CLAY({
            .floating = {
                .attachTo = CLAY_ATTACH_TO_ROOT,
                .attachPoints = {
                    .element = CLAY_ATTACH_POINT_LEFT_TOP,
                    .parent = CLAY_ATTACH_POINT_LEFT_TOP
                }
            }
        }) {
            int size = snprintf(NULL, 0, "FPS: %.2f", 1.0f / deltaTime);
            char *tempBuf = malloc(size + 1);
            if (tempBuf){
                sprintf(tempBuf, "FPS: %.2f", 1.0f / deltaTime);
                Clay_String str = (Clay_String){.chars = tempBuf, .length = size};
                CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 0 }));
                free(tempBuf);
            }
        }
        */

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (top != NULL) {
            C2D_TargetClear(top, C2D_Color32(22, 30, 39, 255));
            C2D_SceneBegin(top);

            Clay_Citro2d_Render(&renderCommands, fonts, GFX_TOP);
        }

        if (bottom != NULL) {
            C2D_TargetClear(bottom, C2D_Color32(22, 30, 39, 255));
            C2D_SceneBegin(bottom);

            C2D_ViewTranslate(-(TOP_WIDTH - BOTTOM_WIDTH) / 2.0f, -TOP_HEIGHT);
            Clay_Citro2d_Render(&renderCommands, fonts, GFX_BOTTOM);
            C2D_ViewReset();
        }

		C3D_FrameEnd(0);
    }

    Scene* current = get_current_scene();
    if (current != NULL) {
        current->unload();
    }

    C2D_FontFree(fonts[0]);

    Clay_Citro2d_Deinit();

    bs_client_free();

	romfsExit();
    socExit();

    return 0;
}