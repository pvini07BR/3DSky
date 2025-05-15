#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "thirdparty/clay/clay.h"
#include <stdbool.h>

typedef struct {
    Clay_String hintText;
    char* textToEdit;
    bool isPassword;
    int maxLength;
    bool disable;
} TextEditData;

void textedit_component(TextEditData* data);

void HandleTextEditInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData);

#endif
