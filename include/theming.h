#ifndef THEMING_H
#define THEMING_H

#include "thirdparty/clay/clay.h"

typedef struct {
    Clay_Color textColor;
    Clay_Color diminishedTextColor;

    Clay_Color backgroundColor;
    Clay_Color accentColor;

    Clay_Color popupBackgroundColor;

    Clay_Color textEditNormalColor;
    Clay_Color textEditHoveredColor;
    Clay_Color textEditHintTextColor;
    Clay_Color textEditNormalTextColor;

    Clay_Color buttonNormalColor;
    Clay_Color buttonHoveredColor;
    Clay_Color buttonDisabledColor;
} Theme;

Theme* get_current_theme();

#endif