#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include "clay/clay.h"
#include <stdbool.h>

/**
 * @brief Estrutura de dados para o componente de edição de texto
 */
typedef struct {
    Clay_String hintText;      /**< Texto de dica a ser exibido quando o campo estiver vazio */
    char* textToEdit;          /**< Ponteiro para o texto a ser editado */
    bool isPassword;           /**< Se o campo é para senha (exibe asteriscos) */
    int maxLength;             /**< Comprimento máximo do texto */
    bool disable;              /**< Se o campo está desabilitado */
} TextEditData;

/**
 * @brief Renderiza um componente de edição de texto
 * 
 * @param id Identificador único do campo de texto
 * @param data Dados do campo de texto
 */
void textedit_component(Clay_String id, TextEditData* data);

/**
 * @brief Função de callback para interação com o campo de texto
 * 
 * @param elementId ID do elemento
 * @param pointerInfo Informações do ponteiro
 * @param userData Dados do usuário (TextEditData*)
 */
void HandleTextEditInteraction(Clay_ElementId elementId, Clay_PointerData pointerInfo, intptr_t userData);

#endif /* TEXTEDIT_H */ 