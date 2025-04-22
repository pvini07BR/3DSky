#include "components/popup.h"
#include "components/button.h"
#include "defines.h"
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static enum PopupType popupType = POPUP_TYPE_MESSAGE;

static char* popup_message_text = NULL;
static size_t popup_message_length = 0;
static bool show_popup_flag = false;

static void (*popup_on_confirm)(void*) = NULL;

void close_popup(void* args) {
    show_popup_flag = false;
    if (popup_message_text) {
        free(popup_message_text);
        popup_message_text = NULL;
        popup_message_length = 0;
    }
}

void close_and_confirm_popup(void* args) {
    close_popup(NULL);
    if (popup_on_confirm != NULL) {
        popup_on_confirm(NULL);
        popup_on_confirm = NULL;
    }
}

void popup_component(Clay_String text, bool bottomScreen) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .padding = {
                .top = bottomScreen ? TOP_HEIGHT : 0,
                .bottom = bottomScreen ? 0 : BOTTOM_HEIGHT,
                .left = bottomScreen ? 0 : TOP_BOTTOM_DIFF,
                .right = bottomScreen ? 0 : TOP_BOTTOM_DIFF
            },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            }
        },
        .floating = {
            .attachTo = CLAY_ATTACH_TO_PARENT,
            .attachPoints = {
                .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                .parent = CLAY_ATTACH_POINT_CENTER_CENTER
            }
        },
        .backgroundColor = {0, 0, 0, 128}
    }) {
        CLAY({
            .id = CLAY_ID("popup"),
            .layout = {
                .padding = CLAY_PADDING_ALL(10),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = {
                    .width = CLAY_SIZING_GROW(.max = TOP_WIDTH / 1.5f)
                },
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER
                },
                .childGap = 10
            },
            .cornerRadius = {.topLeft = 8 },
            .backgroundColor = {30, 41, 53, 255}
        }) {
            CLAY_TEXT(text, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 16, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            if (popupType != POPUP_TYPE_PROGRESS) {
                CLAY({
                    .layout = {
                        .childAlignment = {
                            .x = CLAY_ALIGN_X_CENTER,
                            .y = CLAY_ALIGN_Y_CENTER
                        },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .childGap = 10
                    }
                }) {
                    button_component(
                        CLAY_STRING("closePopup"),
                        popupType == POPUP_TYPE_CONFIRM ? CLAY_STRING("No") : CLAY_STRING("Ok"),
                        false,
                        close_popup
                    );
                    if (popupType == POPUP_TYPE_CONFIRM || popupType == POPUP_TYPE_ERROR) {
                        button_component(
                            CLAY_STRING("confirmPopup"),
                            popupType == POPUP_TYPE_ERROR ? CLAY_STRING("Retry") : CLAY_STRING("Yes"),
                            false,
                            close_and_confirm_popup
                        );
                    }
                }
            }
        }
    }
}

void show_popup_message(const char* message, enum PopupType type, void (*onConfirm)(void*)) {
    if (message == NULL) {
        return;
    }

    size_t new_length = strlen(message);
    char *new_message = malloc(new_length + 1);
    if (!new_message) {
        fprintf(stderr, "Error: Failed to allocate memory for the popup text.\n");
        return;
    }

    strncpy(new_message, message, new_length);
    new_message[new_length] = '\0';

    if (popup_message_text) {
        free(popup_message_text);
    }

    popup_message_text = new_message;
    popup_message_length = new_length;
    show_popup_flag = true;
    popupType = type;
    popup_on_confirm = onConfirm;
}

bool is_popup_visible(void) {
    return show_popup_flag;
}

void render_current_popup(bool bottomScreen) {
    if (show_popup_flag && popup_message_text) {
        Clay_String popup_text = { .length = popup_message_length, .chars = popup_message_text };
        popup_component(popup_text, bottomScreen);
    }
}