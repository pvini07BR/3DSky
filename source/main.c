#include "3ds/romfs.h"
#include "3ds/services/hid.h"
#include "3ds/types.h"
#include "c3d/renderqueue.h"
#include "scenes/scene.h"
#include "theming.h"
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#define CLAY_IMPLEMENTATION
#include "thirdparty/clay/clay.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"

#include "thirdparty/bluesky/bluesky.h"

#include "scenes/login_scene.h"

#include "defines.h"
#include "avatar_img_cache.h"

typedef enum {
    OFF,
    TOP,
    BOTTOM
} ConsoleMode;

// Change this value to either make the console appear on the top screen,
// bottom screen or neither.
const ConsoleMode CONSOLE_MODE = TOP;

void HandleClayErrors(Clay_ErrorData errorData) {
    fprintf(stderr, "[CLAY ERROR]: %s\n", errorData.errorText.chars);
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

    if (CONSOLE_MODE == TOP) {
        consoleInit(GFX_TOP, NULL);
        bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    } else if (CONSOLE_MODE == BOTTOM) {
        consoleInit(GFX_BOTTOM, NULL);
        top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    } else {
        top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
        bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    }
    
    // I don't know if those directories will be needed but there it is
    //if (mkdir(APP_PATH, 0777) != 0 && errno != EEXIST) { perror("Failed creating 3dsky directory"); }
    //if (mkdir(CACHE_PATH, 0777) != 0 && errno != EEXIST) { perror("Failed creating 3dsky/cache directory"); }

    C2D_Font fonts[2];
    fonts[0] = C2D_FontLoadSystem(CFG_REGION_USA);

    C2D_FontSetFilter(fonts[0], GPU_LINEAR, GPU_LINEAR);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions) { TOP_WIDTH, TOP_HEIGHT + BOTTOM_HEIGHT }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(MeasureText, &fonts);

    avatar_img_cache_init();

    change_scene(get_login_scene()); 

    touchPosition tempPos = {-1};
    Clay_Vector2 lastTouchPos = {-1.0f, -1.0f};

    bool set = false;

    u64 lastTime = osGetTime();
    while(aptMainLoop()) {
        hidScanInput();
        
		u32 kDown = hidKeysDown();
        
		if (kDown & KEY_START)
            break;
    
        u64 currentTime = osGetTime();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        touchPosition touch = {-1};
        hidTouchRead(&touch);
        
        if (touch.px != 0 && touch.py != 0) {
            tempPos = touch;
        }

        float touch_x = tempPos.px + TOP_BOTTOM_DIFF;
        float touch_y = tempPos.py + TOP_HEIGHT;

        u32 kHeld = hidKeysHeld();

        if (kHeld & KEY_TOUCH) {
            Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, true);
            if (lastTouchPos.x != -1.0f && lastTouchPos.y != -1.0f) {
                Clay_Vector2 scrollDelta = (Clay_Vector2) {touch_x - lastTouchPos.x, touch_y - lastTouchPos.y};
                Clay_UpdateScrollContainers(true, scrollDelta, deltaTime);  
            } else
                Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, 0}, deltaTime);
            lastTouchPos = (Clay_Vector2) {touch_x, touch_y};
            set = false;
        } else {
            if (!set) {
                Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, false);
                set = true;
            } else {
                Clay_SetPointerState((Clay_Vector2) {INFINITY, INFINITY}, false);
            }
            Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, 0}, deltaTime);
            lastTouchPos = (Clay_Vector2) {-1.0f, -1.0f};
        }

        Scene* current = get_current_scene();
        if (current != NULL) current->update(deltaTime);

        Clay_BeginLayout();

        if (current != NULL) current->layout(deltaTime);

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (top != NULL) {
            C2D_TargetClear(top, ClayColor_to_C2DColor(get_current_theme()->backgroundColor));
            C2D_SceneBegin(top);

            Clay_Citro2d_Render(&renderCommands, fonts, GFX_TOP);
        }

        if (bottom != NULL) {
            C2D_TargetClear(bottom, ClayColor_to_C2DColor(get_current_theme()->backgroundColor));
            C2D_SceneBegin(bottom);

            C2D_ViewTranslate(-TOP_BOTTOM_DIFF, -TOP_HEIGHT);
            Clay_Citro2d_Render(&renderCommands, fonts, GFX_BOTTOM);
            C2D_ViewReset();
        }

		C3D_FrameEnd(0);
    }

    Scene* current = get_current_scene();
    if (current != NULL) {
        current->unload();
    }

    avatar_img_cache_free();

    C2D_FontFree(fonts[0]);

    Clay_Citro2d_Deinit();

    bs_client_free();

	romfsExit();
    socExit();

    return 0;
}
