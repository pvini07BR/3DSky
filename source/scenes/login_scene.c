#include "scenes/login_scene.h"
#include <stdio.h>
#include "c2d/spritesheet.h"
#include "clay/clay.h"
#include "defines.h"
#include "scenes/scene.h"
#include "scenes/main_scene.h"
#include "components/button.h"
#include "components/textedit.h"
#include "components/popup.h"

#include <stdlib.h>
#include <sys/select.h>
#include <curl/curl.h>
#include <string.h>
#include <3ds.h>

#include <jansson.h>

#include "bluesky/bluesky.h"

static Scene login_scene;

bool logging_in = false;
bool login_successful = false;

static char username[80];
static char password[80];

TextEditData handleData = { 
    .hintText = CLAY_STRING("Enter your username handle"), 
    .textToEdit = username, 
    .isPassword = false,
    .maxLength = 80,
    .disable = false
};

TextEditData passwordData = { 
    .hintText = CLAY_STRING("Enter your password"), 
    .textToEdit = password, 
    .isPassword = true,
    .maxLength = 80,
    .disable = false
};

Thread threadHandle;
Handle threadRequest;
bool runThread = true;

C2D_SpriteSheet logoSpriteSheet;
C2D_Image logoImage;

void on_login_button_pressed() {
    if (strlen(username) == 0 || strlen(password) == 0) {
        show_popup_message("Error: Username or password is empty", NULL);
        return;
    }

    if (logging_in || is_popup_visible() || login_successful) {
        return;
    }

    logging_in = true;
    
    svcSignalEvent(threadRequest);
}

void threadMain(void *arg) {
	while(runThread) {
		svcWaitSynchronization(threadRequest, U64_MAX);
		svcClearEvent(threadRequest);

        char error_msg[256];
        int code = bs_client_init(username, password, error_msg);
        if (code != 0) {
            show_popup_message(error_msg, NULL);
            logging_in = false;
        } else {
            login_successful = true;
        }
    }
}

static void login_init(void) {
    logoSpriteSheet = C2D_SpriteSheetLoad("romfs:/3dsky-icon-alt.t3x");
    if (logoSpriteSheet) {
        logoImage = C2D_SpriteSheetGetImage(logoSpriteSheet, 0);
    }

    svcCreateEvent(&threadRequest,0);
	threadHandle = threadCreate(threadMain, 0, (16 * 1024), 0x3f, -2, true);
}

static void login_layout(void) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER}
        },
    }) {
        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_FIXED(TOP_WIDTH), CLAY_SIZING_FIXED(TOP_HEIGHT)},
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
            },
        }) {
            CLAY({
                .layout = { .sizing = { .width = CLAY_SIZING_FIXED(256), .height = CLAY_SIZING_FIXED(128) }},
                .image = {
                    .imageData = &logoImage,
                    .sourceDimensions = (Clay_Dimensions){.width = 256, .height = 128},
                }
            });
        }
        CLAY({
            .layout = {
                .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = BOTTOM_HEIGHT / 6
            },
        }) {
            textedit_component(CLAY_STRING("handle"), &handleData);
            textedit_component(CLAY_STRING("password"), &passwordData);
            if (!logging_in) {
                button_component(CLAY_STRING("login"), CLAY_STRING("Login"), logging_in || is_popup_visible() || login_successful, on_login_button_pressed);
            } else {
                button_component(CLAY_STRING("login"), CLAY_STRING("Logging in..."), logging_in, NULL);
            }
        }
    }

    render_current_popup(true);
}

static void login_update(void) {
    handleData.disable = logging_in || is_popup_visible() || login_successful;
    passwordData.disable = logging_in || is_popup_visible() || login_successful;

    if (login_successful) {
        login_successful = false;
        change_scene(get_main_scene());
        return;
    }

    check_popup_close_button();
}

static void login_unload(void) {
    if (logoSpriteSheet) {
        C2D_SpriteSheetFree(logoSpriteSheet);
    }

    if (login_successful) {
        runThread = false;
        svcCloseHandle(threadRequest);
        return;
    }

    runThread = false;
    svcCloseHandle(threadRequest);
}

Scene* get_login_scene(void) {
    login_scene.init = login_init;
    login_scene.update = login_update;
    login_scene.layout = login_layout;
    login_scene.unload = login_unload;
    
    return &login_scene;
}