#include "components/button.h"
#include <3ds.h>

static void HandleButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        void (*onClick)(void*) = (void (*)(void*))userData;
        if (onClick != NULL) {
            onClick(NULL);
        }
    }
}

void button_component(Clay_String id, Clay_String text, bool disabled, void (*onClick)(void*)) {
    CLAY({
        .id = CLAY_SID(id),
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
        .backgroundColor = disabled ? (Clay_Color) {128, 128, 128, 255} : (Clay_Color) {32, 138, 254, 255}
    }) {
        if (!disabled && onClick != NULL) {
            Clay_OnHover(HandleButtonInteraction, (intptr_t)onClick);
        }
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({ .textColor = {255, 255, 255, 255}, .fontSize = 16, .fontId = 0 }));
    }
} 