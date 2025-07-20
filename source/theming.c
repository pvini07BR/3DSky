#include "theming.h"

static Theme currentTheme = {
    .textColor = {255, 255, 255, 255},
    .diminishedTextColor = {128, 128, 128, 255},

    .backgroundColor = {22, 30, 39, 255},
    .accentColor = {46, 64, 82, 255},

    .popupBackgroundColor = {30, 41, 53, 255},

    .textEditNormalColor = {30, 41, 53, 255},
    .textEditHoveredColor = {40, 52, 64, 255},
    .textEditHintTextColor = {117, 138, 161, 255},
    .textEditNormalTextColor = {241, 243, 244, 255},

    .buttonNormalColor = {32, 138, 254, 255},
    .buttonHoveredColor = {76, 162, 254, 255},
    .buttonDisabledColor = {128, 128, 128, 255}
};

Theme* get_current_theme() { return &currentTheme; }