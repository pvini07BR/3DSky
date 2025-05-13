#include "components/button.h"
#include <3ds.h>
#include "stdio.h"

const Clay_Color BUTTON_NORMAL_COLOR = (Clay_Color) {32, 138, 254, 255};
const Clay_Color BUTTON_HOVERED_COLOR = (Clay_Color) {76, 162, 254, 255};
const Clay_Color BUTTON_DISABLED_COLOR = (Clay_Color) {128, 128, 128, 255};

static void HandleButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
        void (*onClick)(void*) = (void (*)(void*))userData;
        if (onClick != NULL) {
            onClick(NULL);
        }
    }
}

void button_component(Clay_String text, bool disabled, void (*onClick)(void*)) {
    CLAY({
        .layout = {
            .padding = {
                .top = 6,
                .right = 10,
                .bottom = 6,
                .left = 10
            },
            .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_CENTER
            },
        },
        .cornerRadius = {.topLeft = 8 },
        .backgroundColor = disabled ? BUTTON_DISABLED_COLOR : Clay_Hovered() ? BUTTON_HOVERED_COLOR : BUTTON_NORMAL_COLOR,
    }) {
        if (!disabled && onClick != NULL) {
            Clay_OnHover(HandleButtonInteraction, (intptr_t)onClick);
        }
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 16, .fontId = 0 }));
    }
} 
