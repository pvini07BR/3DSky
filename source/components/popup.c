#include "components/popup.h"
#include "components/button.h"
#include "defines.h"
#include <3ds.h>
#include <string.h>
#include <stdlib.h>

// Variáveis globais para o popup
static char popup_message_text[256] = "";
static size_t popup_message_length = 0;
static bool show_popup_flag = false;
static void (*popup_on_close)(void) = NULL;

void popup_component(Clay_String text, bool bottomScreen) {
    CLAY({
        .layout = {
            .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)},
            .padding = {
                .top = bottomScreen ? TOP_HEIGHT : 0,
                .bottom = bottomScreen ? 0 : BOTTOM_HEIGHT
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
            button_component(CLAY_STRING("closePopup"), CLAY_STRING("Ok"), false, NULL);
        }
    }
}

void show_popup_message(const char* message, void (*onClose)(void)) {
    if (message == NULL) {
        return;
    }
    
    strncpy(popup_message_text, message, sizeof(popup_message_text) - 1);
    popup_message_text[sizeof(popup_message_text) - 1] = '\0';
    popup_message_length = strlen(popup_message_text);
    show_popup_flag = true;
    popup_on_close = onClose;
}

void close_popup(void) {
    show_popup_flag = false;
    if (popup_on_close != NULL) {
        popup_on_close();
        popup_on_close = NULL;
    }
}

bool is_popup_visible(void) {
    return show_popup_flag;
}

// Função para verificar se o botão de fechar o popup foi clicado
bool check_popup_close_button(void) {
    u32 kDown = hidKeysDown();
    if (show_popup_flag && kDown & KEY_TOUCH && Clay_PointerOver(CLAY_ID("closePopup"))) {
        close_popup();
        return true;
    }
    return false;
}

// Função para renderizar o popup atual
void render_current_popup(bool bottomScreen) {
    if (show_popup_flag) {
        Clay_String popup_text = { .length = popup_message_length, .chars = popup_message_text };
        popup_component(popup_text, bottomScreen);
    }
} 