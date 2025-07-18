#ifndef POPUP_H
#define POPUP_H

#include "thirdparty/clay/clay.h"
#include <stdbool.h>

// Message: text with an "Ok button"
// Progress: text with or without a progress bar and no buttons
// Confirm: text with "Yes" and "No" buttons
// Error: text with an "Ok" and a "Retry" button
enum PopupType {
    POPUP_TYPE_MESSAGE,
    POPUP_TYPE_PROGRESS,
    POPUP_TYPE_CONFIRM,
    POPUP_TYPE_ERROR,
};

void close_popup(void* args);
void popup_layout(bool bottomScreen);
// The onConfirm function pointer is going to be used for both the confirm and error popups
void show_popup_message(const char* message, enum PopupType type, void (*onConfirm)(void*));
bool is_popup_visible(void);

void popup_set_progress_bar(float value);

#endif