#ifndef POPUP_H
#define POPUP_H

#include "clay/clay.h"
#include <stdbool.h>

typedef struct {
    char* message;
    size_t messageLength; 
    bool isVisible;
    void (*onClose)(void);
} PopupData;

void popup_component(Clay_String text, bool bottomScreen);
void show_popup_message(const char* message, void (*onClose)(void));
void close_popup(void);
bool is_popup_visible(void);
bool check_popup_close_button(void);
void render_current_popup(bool bottomScreen);

#endif