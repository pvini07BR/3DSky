#ifndef BUTTON_H
#define BUTTON_H

#include "clay/clay.h"

/**
 * @brief Renderiza um componente de botão
 * 
 * @param id Identificador único do botão
 * @param text Texto a ser exibido no botão
 * @param disabled Se o botão está desabilitado
 * @param onClick Callback a ser chamado quando o botão for clicado (opcional)
 */
void button_component(Clay_String id, Clay_String text, bool disabled, void (*onClick)(void*));

#endif /* BUTTON_H */ 