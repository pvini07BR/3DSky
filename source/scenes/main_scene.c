#include <jansson.h>
#include <citro2d.h>
#include "c2d/spritesheet.h"
#include "components/feed.h"
#include "theming.h"
#include "thirdparty/bluesky/bluesky.h"

#include "scenes/main_scene.h"
#include "pages/profile.h"
#include "pages/timeline.h"

#include "components/popup.h"
#include "thirdparty/clay/clay_renderer_citro2d.h"

static Scene main_scene;

Pages currentPage = HOME;

C2D_SpriteSheet navBarIconsSheet = NULL;
C2D_SpriteSheet postIconsSheet = NULL;

const unsigned int navIconsCount = 2;
C2D_Image navIcons[2];

C2D_Image repliesIcon;
C2D_Image repostIcon;
C2D_Image likeIcon;

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

void main_scene_change_to_profile(const char* handle) {
    if (handle == NULL) return;

    profileData.feed.setScroll = true;
    currentPage = PROFILE;

    if (navBarIconsSheet) {
        navIcons[HOME] = C2D_SpriteSheetGetImage(navBarIconsSheet, 0);
        navIcons[PROFILE] = C2D_SpriteSheetGetImage(navBarIconsSheet, 3);
    }

    Clay_Citro2d_ClearTextCacheAndBuffer();

    if (!profileData.initialized)
        profile_page_init(&profileData, handle, &repliesIcon, &repostIcon, &likeIcon);
    else
        profile_page_load(&profileData, handle);
}

void main_scene_change_page(Pages page) {
    switch (page) {
    case HOME:
        timelineData.feed.setScroll = true;
        currentPage = page;

        if (navBarIconsSheet) {
            navIcons[HOME] = C2D_SpriteSheetGetImage(navBarIconsSheet, 1);
            navIcons[PROFILE] = C2D_SpriteSheetGetImage(navBarIconsSheet, 2);
        }

        Clay_Citro2d_ClearTextCacheAndBuffer();

        if (!timelineData.initialized)
            timeline_init(&timelineData, &repliesIcon, &repostIcon, &likeIcon);
        break;
    case PROFILE:
        main_scene_change_to_profile(bs_client_get_current_handle());
        break;
    default:
        show_popup_message("This has not been implemented yet!", POPUP_TYPE_MESSAGE, NULL);
        break;
    }
}

void handleNavButton(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    Pages page = (Pages)userData;

    if (!nav_button_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
        nav_button_pressed = true;
    }

    if (nav_button_pressed && hidKeysUp() & KEY_TOUCH) {
        nav_button_pressed = false;
        main_scene_change_page(page);
    }
}

// TODO: Add more pages (notifications, chat, profile, etc.)
static void main_init() {
    navBarIconsSheet = C2D_SpriteSheetLoad("romfs:/navBarIcons.t3x");
    postIconsSheet = C2D_SpriteSheetLoad("romfs:/postIcons.t3x");

    if (postIconsSheet) {
        repliesIcon = C2D_SpriteSheetGetImage(postIconsSheet, 0);
        repostIcon = C2D_SpriteSheetGetImage(postIconsSheet, 1);
        likeIcon = C2D_SpriteSheetGetImage(postIconsSheet, 2);
    }
    
    if (currentPage != HOME) return;

    if (navBarIconsSheet) {
        navIcons[HOME] = C2D_SpriteSheetGetImage(navBarIconsSheet, 1);
        navIcons[PROFILE] = C2D_SpriteSheetGetImage(navBarIconsSheet, 2);
    }
    timeline_init(&timelineData, &repliesIcon, &repostIcon, &likeIcon);
}

static void main_layout(float deltaTime) {
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
                    timeline_page_layout(&timelineData, deltaTime);
                    break;
                case PROFILE:
                    profile_page_layout(&profileData, deltaTime);
                    break;
                default:
                    break;
            }
        }
     
        // Nav bar
        CLAY((Clay_ElementDeclaration){
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(BOTTOM_WIDTH + TOP_BOTTOM_DIFF), CLAY_SIZING_FIT()},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = BOTTOM_WIDTH - (iconDimensions.width * navIconsCount) - (iconDimensions.width * (navIconsCount * 2)),
                .padding = {.left = TOP_BOTTOM_DIFF, .bottom = 5, .top = 5}
            },
            .border = {.width = {.top= 1 }, .color = get_current_theme()->accentColor},
        }) {
            for (int i = 0; i < navIconsCount; i++) {
                CLAY((Clay_ElementDeclaration){
                    .layout = iconLayout,
                    .image = {
                        .imageData = &navIcons[i],
                    },
                    .aspectRatio = { iconDimensions.width / iconDimensions.height },
                    .backgroundColor = get_current_theme()->textColor
                }) {
                    Clay_OnHover(handleNavButton, i);
                }
            }
        }
    }

    popup_layout(true);
}

static void main_update(float deltaTime) {
    timeline_update(&timelineData, deltaTime);
}

static void main_unload(void) {
    timeline_free(&timelineData);
    profile_page_free(&profileData);

    if (navBarIconsSheet) C2D_SpriteSheetFree(navBarIconsSheet);
}

Scene* get_main_scene(void) {
    main_scene.init = main_init;
    main_scene.update = main_update;
    main_scene.layout = main_layout;
    main_scene.unload = main_unload;
    
    return &main_scene;
} 
