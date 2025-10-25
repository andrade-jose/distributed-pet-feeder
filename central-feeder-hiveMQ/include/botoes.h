#pragma once
#include <Arduino.h>
#include "config.h"

// Classe para gerenciar os botões
class Botoes {
private:
    static const unsigned long delayDebounce = BUTTON_DEBOUNCE_MS;

    // Função auxiliar para leitura com debounce
    static bool botaoPressionado(uint8_t pino, unsigned long &ultimoPressionamento);

public:
        static bool qualquerBotaoPressionado() {
        return cimaPressionado() || baixoPressionado() || enterPressionado();
    }
    // Inicializar configurações dos botões
    static void inicializar();

    // Verificar botões individuais
    static bool cimaPressionado();
    static bool baixoPressionado();
    static bool enterPressionado();
};
