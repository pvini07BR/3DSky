#include "scenes/main_scene.h"
#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "clay/clay_renderer_citro2d.h"
#include "defines.h"
#include "scenes/scene.h"

#include <malloc.h>

#include "bluesky/bluesky.h"

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

    change_scene(get_main_scene());

    Clay_Vector2 lastTouchPos = {-1.0f, -1.0f};

    while(aptMainLoop()) {
        hidScanInput();

        u32 kHeld = hidKeysHeld();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break;

        touchPosition touch = {-1};
        hidTouchRead(&touch);

        if (kHeld & KEY_TOUCH) {
            float touch_x = touch.px + TOP_BOTTOM_DIFF;
            float touch_y = touch.py + TOP_HEIGHT;

            Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, kHeld & KEY_TOUCH);
            if (lastTouchPos.x != -1.0f && lastTouchPos.y != -1.0f) {
                Clay_Vector2 scrollDelta = (Clay_Vector2) {touch_x - lastTouchPos.x, touch_y - lastTouchPos.y};
                Clay_UpdateScrollContainers(true, scrollDelta, 1.0 / 60.0f);
                lastTouchPos = (Clay_Vector2) {touch_x, touch_y};
            } else {
                lastTouchPos = (Clay_Vector2) {touch_x, touch_y};
                Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, 0}, 1.0 / 60.0f);
            }
        } else {
            Clay_SetPointerState((Clay_Vector2) {-100.0f, -100.0f}, false);
            Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, 0}, 1.0 / 60.0f);
            lastTouchPos = (Clay_Vector2) {-1.0f, -1.0f};
        }

        Scene* current = get_current_scene();
        if (current != NULL) {
            current->update();
        }

        Clay_BeginLayout();

        if (current != NULL) {
            current->layout();
        }

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