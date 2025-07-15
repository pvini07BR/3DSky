#include "components/popup.h"
#include "components/button.h"
#include "defines.h"
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static enum PopupType popupType = POPUP_TYPE_MESSAGE;

static char* popup_text = NULL;
static int popup_text_length = 0;

static bool show_popup_flag = false;

static void (*popup_on_confirm)(void*) = NULL;

static float progress_bar_value = 0.0f;

void close_popup(void* args) {
    show_popup_flag = false;
    progress_bar_value = 0.0f;
}

void close_and_confirm_popup(void* args) {
    close_popup(NULL);
    if (popup_on_confirm != NULL) {
        popup_on_confirm(NULL);
        popup_on_confirm = NULL;
    }
}

void popup_layout(bool bottomScreen) {
    if (!show_popup_flag) return;

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
            .attachTo = CLAY_ATTACH_TO_ROOT,
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
            if (popup_text != NULL) {
                Clay_String str = (Clay_String) {
                    .chars = popup_text,
                    .length = popup_text_length,
                    .isStaticallyAllocated = false
                };

                CLAY_TEXT(str, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 16, .fontId = 0, .textAlignment = CLAY_TEXT_ALIGN_CENTER }));
            }
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
                        popupType == POPUP_TYPE_CONFIRM ? CLAY_STRING("No") : CLAY_STRING("Ok"),
                        false,
                        close_popup
                    );
                    if (popupType == POPUP_TYPE_CONFIRM || popupType == POPUP_TYPE_ERROR) {
                        button_component(
                            popupType == POPUP_TYPE_ERROR ? CLAY_STRING("Retry") : CLAY_STRING("Yes"),
                            false,
                            close_and_confirm_popup
                        );
                    }
                }
            } else {
                CLAY({
                    .layout = {
                        .sizing = {
                            .width = CLAY_SIZING_GROW(0),
                            .height = CLAY_SIZING_FIXED(30)
                        },
                        .childAlignment = {
                            .x = CLAY_ALIGN_X_LEFT,
                            .y = CLAY_ALIGN_Y_CENTER
                        },
                        .padding = CLAY_PADDING_ALL(2)
                    },
                    .border = {
                        .color = {255, 255, 255, 255},
                        .width = {
                            .top = 1,
                            .bottom = 1,
                            .left = 1,
                            .right = 1
                        }
                    },
                }) {
                    CLAY({
                        .layout = {
                            .sizing = {
                                .width = CLAY_SIZING_PERCENT(progress_bar_value),
                                .height = CLAY_SIZING_GROW(0)
                            }
                        },
                        .backgroundColor = {255, 255, 255, 255},
                    });
                }
            }
        }
    }
}

void show_popup_message(const char* message, enum PopupType type, void (*onConfirm)(void*)) {
    if (message == NULL) {
        return;
    }

    if (popup_text != NULL) {
        free(popup_text);
    }

    popup_text_length = strlen(message);
    popup_text = malloc(popup_text_length + 1);
    if (!popup_text) {
        fprintf(stderr, "Error: Failed to allocate memory for the popup text.\n");
        return;
    }

    strncpy(popup_text, message, popup_text_length);
    popup_text[popup_text_length] = '\0';

    show_popup_flag = true;
    popupType = type;
    popup_on_confirm = onConfirm;
}

void popup_set_progress_bar(float value) {
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    progress_bar_value = value;
}

bool is_popup_visible(void) {
    return show_popup_flag;
}