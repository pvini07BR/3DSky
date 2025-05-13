#include "3ds/romfs.h"
#include "3ds/types.h"
#include "c3d/renderqueue.h"
#include <malloc.h>
#include <stdio.h>
#include <sys/stat.h>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "clay/clay_renderer_citro2d.h"

#include "bluesky/bluesky.h"

#include "scenes/login_scene.h"
#include "scenes/main_scene.h"

#include "defines.h"

typedef enum {
    OFF,
    TOP,
    BOTTOM
} ConsoleMode;

typedef enum {
    LOGIN,
    MAIN
} SceneEnum;

// You can change the CUR_SCENE constants to quickly get
// to the main scene, without having to go through the login.
//
// To define the login, you need to uncomment the #define DEBUG_LOGIN
// and set your handle and password below.
//
// DON'T FORGET TO REMOVE YOUR CREDENTIALS AFTER TESTING!!!

const ConsoleMode CONSOLE_MODE = OFF;
const SceneEnum CUR_SCENE = LOGIN;

//#define DEBUG_LOGIN

#ifdef DEBUG_LOGIN
    #define DEBUG_HANDLE "user.bsky.social"
    #define DEBUG_PASSWORD "password"
#endif

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

    #ifdef DEBUG_LOGIN
        bs_client_init(DEBUG_HANDLE, DEBUG_PASSWORD, NULL);
    #endif

    switch (CUR_SCENE) {
        case LOGIN:
            change_scene(get_login_scene());
            break;
        case MAIN:
            change_scene(get_main_scene());
            break;
    }

    touchPosition tempPos = {-1};
    Clay_Vector2 lastTouchPos = {-1.0f, -1.0f};

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

        // TODO: Fix the issue of the clay pointer position staying at the last touch position when the touch is released
        
        if ((touch.px != 0 && touch.py != 0) || hidKeysHeld() & KEY_TOUCH || hidKeysDown() & KEY_TOUCH) {
            Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, true);
            if (lastTouchPos.x != -1.0f && lastTouchPos.y != -1.0f) {
                Clay_Vector2 scrollDelta = (Clay_Vector2) {touch_x - lastTouchPos.x, touch_y - lastTouchPos.y};
                Clay_UpdateScrollContainers(true, scrollDelta, deltaTime);                
            } else {
                Clay_UpdateScrollContainers(true, (Clay_Vector2) {0, 0}, deltaTime);
            }
            lastTouchPos = (Clay_Vector2) {touch_x, touch_y};
        } else {
            Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, false);
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

    C2D_FontFree(fonts[0]);

    Clay_Citro2d_Deinit();

    bs_client_free();

	romfsExit();
    socExit();

    return 0;
}
