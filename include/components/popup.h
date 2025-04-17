#ifndef POPUP_H
#define POPUP_H

#include "clay/clay.h"
#include <stdbool.h>

/**
 * @brief Estrutura de dados para o componente de popup
 */
typedef struct {
    char* message;             /**< Mensagem a ser exibida no popup */
    size_t messageLength;      /**< Comprimento da mensagem */
    bool isVisible;            /**< Se o popup está visível */
    void (*onClose)(void);     /**< Callback a ser chamado quando o popup for fechado */
} PopupData;

/**
 * @brief Renderiza um componente de popup
 * 
 * @param text Texto a ser exibido no popup
 */
void popup_component(Clay_String text);

/**
 * @brief Renderiza um overlay de fundo para o popup
 */
void popup_overlay(void);

/**
 * @brief Exibe um popup com a mensagem especificada
 * 
 * @param message Mensagem a ser exibida
 * @param onClose Callback a ser chamado quando o popup for fechado (opcional)
 */
void show_popup_message(const char* message, void (*onClose)(void));

/**
 * @brief Fecha o popup atual
 */
void close_popup(void);

/**
 * @brief Verifica se o popup está visível
 * 
 * @return true Se o popup está visível
 * @return false Se o popup não está visível
 */
bool is_popup_visible(void);

/**
 * @brief Verifica se o botão de fechar o popup foi clicado
 * 
 * @return true Se o botão foi clicado
 * @return false Se o botão não foi clicado
 */
bool check_popup_close_button(void);

/**
 * @brief Renderiza o popup atual
 */
void render_current_popup(void);

#endif /* POPUP_H */ 