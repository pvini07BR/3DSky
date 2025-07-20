#include <jansson.h>
#include <citro2d.h>
#include "components/feed.h"
#include "theming.h"
#include "thirdparty/bluesky/bluesky.h"

#include "scenes/main_scene.h"
#include "pages/profile.h"
#include "pages/timeline.h"

#include "components/popup.h"

static Scene main_scene;

typedef enum {
    HOME = 0,
    SEARCH = 1,
    CHAT = 2,
    NOTIFICATIONS = 3,
    PROFILE = 4
} Pages;

Pages currentPage = HOME;

// So, there was a issue where the program would crash or the JSON strings would get messed up
// if both the timeline and the profile were being loaded at the same time. The simplest and 
// stupidest solution to this problem was to just simply disable the nav buttons while the
// current page is being loaded.
bool disableNavButtons = true;

C2D_SpriteSheet iconsSheet = NULL;
C2D_SpriteSheet otherIconsSheet = NULL;

C2D_Image navIcons[5];

Clay_LayoutConfig iconLayout = {
    .sizing = {
        .width = CLAY_SIZING_FIXED(32),
        .height = CLAY_SIZING_FIXED(32) 
    }
};

Clay_Dimensions iconDimensions = {
    .width = 32,
    .height = 32
};

TimelinePage timelineData;
ProfilePage profileData;

bool nav_button_pressed = false;

void handleNavButton(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    Pages page = (Pages)userData;

    if (!nav_button_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
        nav_button_pressed = true;
    }

    if (nav_button_pressed && hidKeysUp() & KEY_TOUCH) {
        nav_button_pressed = false;
        switch (page) {
        case HOME:
            timelineData.feed.setScroll = true;
            currentPage = page;

            if (!timelineData.initialized)
                timeline_init(&timelineData);
            break;
        case PROFILE:
            profileData.feed.setScroll = true;
            currentPage = page;

            if (!profileData.initialized)
                profile_page_load(&profileData, bs_client_get_current_handle());
            break;
        default:
            show_popup_message("This has not been implemented yet!", POPUP_TYPE_MESSAGE, NULL);
            break;
        }
    }
}

// TODO: Add more pages (notifications, chat, profile, etc.)
static void main_init() {
    iconsSheet = C2D_SpriteSheetLoad("romfs:/icons.t3x");
    if (iconsSheet) {
        navIcons[HOME] = C2D_SpriteSheetGetImage(iconsSheet, 0);
        navIcons[SEARCH] = C2D_SpriteSheetGetImage(iconsSheet, 1);
        navIcons[CHAT] = C2D_SpriteSheetGetImage(iconsSheet, 2);
        navIcons[NOTIFICATIONS] = C2D_SpriteSheetGetImage(iconsSheet, 3);
    }

    otherIconsSheet = C2D_SpriteSheetLoad("romfs:/othericons.t3x");
    if (otherIconsSheet)
        navIcons[PROFILE] = C2D_SpriteSheetGetImage(otherIconsSheet, 0);
    
    switch (currentPage){
        case HOME:
            timeline_init(&timelineData);
            break;
        case PROFILE:
            profile_page_load(&profileData, bs_client_get_current_handle());
        default:
            break;
    };
}

static void main_layout(void) {
    CLAY((Clay_ElementDeclaration){
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        // Page layout
        CLAY((Clay_ElementDeclaration){
            .layout = {
                .sizing = {
                    .width = CLAY_SIZING_GROW(0),
                    .height = CLAY_SIZING_GROW(0)
                }
            }
        }) {
            switch (currentPage) {
                case HOME:
                    timeline_page_layout(&timelineData);
                    break;
                case PROFILE:
                    profile_page_layout(&profileData);
                    break;
                default:
                    break;
            }
        }
     
        // Nav bar
        CLAY((Clay_ElementDeclaration){
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(40)},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 32,
                .padding = {.left = 5, .right = 10}
            },
            .border = {.width = {.top= 1 }, .color = get_current_theme()->accentColor},
        }) {
            for (int i = 0; i < 5; i++) {
                CLAY((Clay_ElementDeclaration){
                    .layout = iconLayout,
                    .image = {
                        .imageData = &navIcons[i],
                    },
                    .aspectRatio = { iconDimensions.width / iconDimensions.height },
                    .backgroundColor = disableNavButtons ? get_current_theme()->diminishedTextColor : get_current_theme()->textColor
                }) {
                    Clay_OnHover(handleNavButton, i);
                }
            }
        }
    }

    popup_layout(true);
}

static void main_update(void) {
    switch(currentPage) {
        case HOME:
            disableNavButtons = !timelineData.feed.loaded;
            break;
        case PROFILE:
            disableNavButtons = !profileData.loaded || !profileData.feed.loaded;
            break;
        default:
            break;
    }

    timeline_update(&timelineData);
}

static void main_unload(void) {
    timeline_free(&timelineData);
    profile_page_free(&profileData);

    if (iconsSheet) {
        C2D_SpriteSheetFree(iconsSheet);
    }
    if (otherIconsSheet) {
        C2D_SpriteSheetFree(otherIconsSheet);
    }
}

Scene* get_main_scene(void) {
    main_scene.init = main_init;
    main_scene.update = main_update;
    main_scene.layout = main_layout;
    main_scene.unload = main_unload;
    
    return &main_scene;
} 
