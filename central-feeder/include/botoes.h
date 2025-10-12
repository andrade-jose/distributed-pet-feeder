#pragma once
#include "config.h"

// Classe para gerenciar os botões
class Botoes {
private:
    static const unsigned long delayDebounce = BUTTON_DEBOUNCE_MS;

public:
    // Inicializar configurações dos botões
    static void inicializar();

    // Verificar botões individuais
    static bool cimaPressionado();
    static bool baixoPressionado();
    static bool enterPressionado();
};
