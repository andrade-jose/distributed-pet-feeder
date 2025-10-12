#pragma once
#include "config.h"

// Classe para gerenciar os botões
class Botoes {
private:
    static unsigned long ultimoPressionamento;
    static const unsigned long delayDebounce = BUTTON_DEBOUNCE_MS;

public:
    // Inicializar configurações dos botões
    static void inicializar();

    // Verificar se um botão específico foi pressionado
    static bool estaPressionado(int pino);

    // Verificar todos os botões e imprimir no Serial
    static void verificarTodos();

    // Verificar botões individuais
    static bool cimaPressionado();
    static bool baixoPressionado();
    static bool enterPressionado();
};
