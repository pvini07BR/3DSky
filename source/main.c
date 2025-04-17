#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "clay/clay_renderer_citro2d.h"
#include "defines.h"
#include "scenes/scene.h"
#include "scenes/login_scene.h"

#include <malloc.h>
#include <sys/select.h>

#include "bluesky/bluesky.h"

enum ConsoleMode {
    OFF,
    TOP,
    BOTTOM
};

enum ConsoleMode consoleMode = TOP;

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s\n", errorData.errorText.chars);
}

int main() {
    romfsInit();
	cfguInit();

    // Initialize SOC
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

    C2D_Font fonts[3];
    fonts[0] = C2D_FontLoadSystem(CFG_REGION_USA);
    fonts[1] = C2D_FontLoad("romfs:/segoeui.bcfnt");

    C2D_FontSetFilter(fonts[0], GPU_LINEAR, GPU_LINEAR);
    C2D_FontSetFilter(fonts[1], GPU_LINEAR, GPU_LINEAR);

    //C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/logo.t3x");
    //C2D_Image logo = C2D_SpriteSheetGetImage(spriteSheet, 0);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions) { TOP_WIDTH, TOP_HEIGHT + BOTTOM_HEIGHT }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(MeasureText, &fonts);

    Clay_Color bgColor = (Clay_Color) {22, 30, 39, 255};

    change_scene(get_login_scene());

    while(aptMainLoop()) {
        hidScanInput();

        u32 kHeld = hidKeysHeld();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START)
			break;
        
        /*
        if (kDown & KEY_A) {
            Scene* current = get_current_scene();
            if (current == get_menu_scene()) {
                change_scene(get_game_scene());
            } else {
                change_scene(get_menu_scene());
            }
        }
        */

        touchPosition touch = {-1};
        hidTouchRead(&touch);

        if (kHeld & KEY_TOUCH) {
            float touch_x = touch.px + TOP_BOTTOM_DIFF;
            float touch_y = touch.py + TOP_HEIGHT;

            Clay_SetPointerState((Clay_Vector2) {touch_x, touch_y}, kHeld & KEY_TOUCH);
        } else {
            Clay_SetPointerState((Clay_Vector2) {-100.0f, -100.0f}, false);
        }

        Clay_BeginLayout();

        CLAY({
            .id = CLAY_ID("all"),
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
        }) {
            CLAY({
                .id = CLAY_ID("top"),
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(TOP_WIDTH), CLAY_SIZING_FIXED(TOP_HEIGHT)},
                },
                .backgroundColor = bgColor
            }) {
                Scene* current = get_current_scene();
                if (current != NULL) {
                    current->layout_top();
                }
                //CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 1 }));
            }

            CLAY({
                .id = CLAY_ID("bottom"),
                .layout = {
                    .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH + TOP_BOTTOM_DIFF), CLAY_SIZING_FIXED(BOTTOM_HEIGHT)},
                    .padding = {.left = TOP_BOTTOM_DIFF },
                },
                .backgroundColor = bgColor
            }) {
                Scene* current = get_current_scene();
                if (current != NULL) {
                    current->layout_bottom();
                }
                /*
                CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 24, .fontId = 1 }));
                CLAY({ .backgroundColor = Clay_Hovered() ? (Clay_Color) {255, 0, 0, 255} : (Clay_Color) {0, 255, 0, 255} }) {
                    CLAY({ .id = CLAY_ID("ProfilePicture"), .layout = { .sizing = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) }}, .image = { .imageData = &logo, .sourceDimensions = {60, 60} } }) {}
                    CLAY_TEXT(CLAY_STRING("Button"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
                }
                */
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        Scene* current = get_current_scene();
        if (current != NULL) {
            current->update();
        }

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (top != NULL) {
            C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(top);

            Clay_Citro2d_Render(&renderCommands, fonts);
        }

        if (bottom != NULL) {
            C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bottom);

            C2D_ViewTranslate(-(TOP_WIDTH - BOTTOM_WIDTH) / 2.0f, -TOP_HEIGHT);
            Clay_Citro2d_Render(&renderCommands, fonts);
            C2D_ViewReset();
        }

		C3D_FrameEnd(0);
    }

    bs_client_free();

    C2D_FontFree(fonts[0]);
    C2D_FontFree(fonts[1]);
    C2D_FontFree(fonts[2]);
    
    Clay_Citro2d_Deinit();

	romfsExit();
	cfguExit();
    socExit();
    return 0;
}