#include "botoes.h"
#include "config.h"
#include <Arduino.h>

// Variáveis estáticas para debounce individual
static unsigned long ultimoPressionamentoCima = 0;
static unsigned long ultimoPressionamentoBaixo = 0;
static unsigned long ultimoPressionamentoEnter = 0;

// Inicializar botões
void Botoes::inicializar() {
    Serial.println("=== Inicializando Botões ===");

    // Configurar pinos como entrada com pull-up interno
    pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENTER_BUTTON_PIN, INPUT_PULLUP);

    Serial.println("Botões inicializados com sucesso");
    Serial.printf("Pino CIMA: %d\n", UP_BUTTON_PIN);
    Serial.printf("Pino BAIXO: %d\n", DOWN_BUTTON_PIN);
    Serial.printf("Pino ENTER: %d\n", ENTER_BUTTON_PIN);
}

// Função auxiliar genérica para leitura com debounce
bool Botoes::botaoPressionado(uint8_t pino, unsigned long &ultimoPressionamento) {
    unsigned long agora = millis();

    // Controle de debounce
    if (agora - ultimoPressionamento < delayDebounce)
        return false;

    // LOW = pressionado (por causa do pull-up interno)
    if (digitalRead(pino) == LOW) {
        ultimoPressionamento = agora;
        #ifdef DEBUG_BOTOES
        Serial.printf("DEBUG: Botão no pino %d pressionado\n", pino);
        #endif
        return true;
    }

    return false;
}

// Funções específicas para cada botão
bool Botoes::cimaPressionado() {
    return botaoPressionado(UP_BUTTON_PIN, ultimoPressionamentoCima);
}

bool Botoes::baixoPressionado() {
    return botaoPressionado(DOWN_BUTTON_PIN, ultimoPressionamentoBaixo);
}

bool Botoes::enterPressionado() {
    return botaoPressionado(ENTER_BUTTON_PIN, ultimoPressionamentoEnter);
}
