#ifndef BUTTON_H
#define BUTTON_H

#include "clay/clay.h"

void button_component(Clay_String text, bool disabled, void (*onClick)(void*));

#endif
