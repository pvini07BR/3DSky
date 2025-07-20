#include "components/button.h"
#include <3ds.h>
#include "3ds/services/hid.h"
#include "stdio.h"
#include "theming.h"

bool button_pressed = false;

void HandleButtonInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    /*
    switch (pointerInfo.state) {
        case CLAY_POINTER_DATA_PRESSED: {
            printf("Pointer position: (%f, %f)\nPointer state: CLAY_POINTER_DATA_PRESSED\n",
                pointerInfo.position.x,
                pointerInfo.position.y
            );
            break;
        }
        case CLAY_POINTER_DATA_PRESSED_THIS_FRAME: {
            printf("Pointer position: (%f, %f)\nPointer state: CLAY_POINTER_DATA_PRESSED_THIS_FRAME\n",
                pointerInfo.position.x,
                pointerInfo.position.y
            );
            break;
        }
        case CLAY_POINTER_DATA_RELEASED: {
            printf("Pointer position: (%f, %f)\nPointer state: CLAY_POINTER_DATA_RELEASED\n",
                pointerInfo.position.x,
                pointerInfo.position.y
            );
            break;
        }
        case CLAY_POINTER_DATA_RELEASED_THIS_FRAME: {
            printf("Pointer position: (%f, %f)\nPointer state: CLAY_POINTER_DATA_RELEASED_THIS_FRAME\n",
                pointerInfo.position.x,
                pointerInfo.position.y
            );
            break;
        }
    }
    */

    // Weird trick to make this thing work
    if (!button_pressed && pointerInfo.state == CLAY_POINTER_DATA_RELEASED) {
        button_pressed = true;
    }

    if (button_pressed && hidKeysUp() & KEY_TOUCH) {
        void (*onClick)(void*) = (void (*)(void*))userData;
        if (onClick != NULL) {
            onClick(NULL);
        }
        button_pressed = false;
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
        .backgroundColor = disabled ? get_current_theme()->buttonDisabledColor : Clay_Hovered() ? get_current_theme()->buttonHoveredColor : get_current_theme()->buttonNormalColor,
    }) {
        if (!disabled && onClick != NULL) {
            Clay_OnHover(HandleButtonInteraction, (intptr_t)onClick);
        }
        CLAY_TEXT(text, CLAY_TEXT_CONFIG({ .textColor = get_current_theme()->textColor, .fontSize = 16, .fontId = 0 }));
    }
} 
