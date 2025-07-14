#include "components/textedit.h"
#include "defines.h"
#include <3ds.h>
#include <string.h>
#include <stdlib.h>

const Clay_Color TEXTEDIT_NORMAL_COLOR = (Clay_Color) {30, 41, 53, 255};
const Clay_Color TEXTEDIT_HOVERED_COLOR = (Clay_Color) {40, 52, 64, 255};

void HandleTextEditInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData) {
    TextEditData* data = (TextEditData*)userData;
    if (data->disable) {
        return;
    }

    if (pointerInfo.state == CLAY_POINTER_DATA_RELEASED_THIS_FRAME) {
        SwkbdState swkbd;
        SwkbdButton button = SWKBD_BUTTON_NONE;

        swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, data->maxLength > 0 ? data->maxLength : 80);
        swkbdSetInitialText(&swkbd, data->textToEdit);
        swkbdSetHintText(&swkbd, data->hintText.chars);
        swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, data->maxLength > 0 ? data->maxLength : 80);
        swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN);

        if (data->isPassword) {
            swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
        }

        char tempBuf[data->maxLength > 0 ? data->maxLength : 80];
        button = swkbdInputText(&swkbd, tempBuf, sizeof(tempBuf));

        if (button == SWKBD_BUTTON_CONFIRM) {
            strcpy(data->textToEdit, tempBuf);

            if (data->isPassword) {
                strcpy(data->obfuscatedText, data->textToEdit);

                for (int i = 0; i < strlen(data->obfuscatedText); i++) {
                    data->obfuscatedText[i] = '*';
                }
            }
        }
    }
}

void textedit_component(TextEditData* data) {
    Clay_String text;

    if (data->isPassword && data->obfuscatedText != NULL) {
        text = (Clay_String){ .length = strlen(data->obfuscatedText), .chars = data->obfuscatedText };
    } else {
        text = (Clay_String){ .length = strlen(data->textToEdit), .chars = data->textToEdit };
    }

    CLAY({
        .layout = {
            .padding = CLAY_PADDING_ALL(10),
            .childAlignment = {.y = CLAY_ALIGN_Y_CENTER },
            .sizing = {CLAY_SIZING_FIXED(TOP_WIDTH / 1.6f)}
        },
        .backgroundColor = Clay_Hovered() ? TEXTEDIT_HOVERED_COLOR : TEXTEDIT_NORMAL_COLOR,
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
