#include "components/textedit.h"
#include "defines.h"
#include <3ds.h>
#include <3ds/applets/swkbd.h>
#include <string.h>
#include <stdlib.h>

void HandleTextEditInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    TextEditData* data = (TextEditData*)userData;

    if (pointerInfo.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        SwkbdState swkbd;
        SwkbdButton button = SWKBD_BUTTON_NONE;

        swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, data->maxLength > 0 ? data->maxLength : 80);
        swkbdSetInitialText(&swkbd, data->textToEdit);
        swkbdSetHintText(&swkbd, data->hintText.chars);
        swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, data->maxLength > 0 ? data->maxLength : 80);

        if (data->isPassword) {
            swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
        }

        char tempBuf[data->maxLength > 0 ? data->maxLength : 80];
        button = swkbdInputText(&swkbd, tempBuf, sizeof(tempBuf));

        if (button == SWKBD_BUTTON_CONFIRM) {
            strcpy(data->textToEdit, tempBuf);
        }
    }
}

void textedit_component(Clay_String id, TextEditData* data) {
    Clay_String text = (Clay_String) { .length = strlen(data->textToEdit), .chars = data->textToEdit };

    if (data->isPassword) {
        char* masked = malloc(text.length);
        if (masked != NULL) {
            for (int i = 0; i < text.length; i++) {
                masked[i] = '*';
            }
            text.chars = masked;
        }
    }

    CLAY({
        .id = CLAY_SID(id),
        .layout = {
            .padding = CLAY_PADDING_ALL(10),
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {CLAY_SIZING_FIXED(TOP_WIDTH / 1.6f)}
        },
        .backgroundColor = (Clay_Color) {30, 41, 53, 255},
        .cornerRadius = {.topLeft = 8 }
    }) {
        Clay_OnHover(HandleTextEditInteraction, (intptr_t)data);
        CLAY_TEXT(
            text.length > 0 ? text : data->hintText,
            CLAY_TEXT_CONFIG({
                .textColor = text.length > 0 ? (Clay_Color) {241, 243, 244, 255} : (Clay_Color) {117, 138, 161, 255},
                .fontSize = 16,
                .fontId = 0
            })
        );
    }
} 